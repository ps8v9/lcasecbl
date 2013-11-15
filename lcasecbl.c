#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum contexts {
    CODE,
    LITERAL,
    PSEUDOTEXT
};

enum errors {
    CR_WITHOUT_LF = 1
};

const int sequence_area      =  0; /* start of sequence area */
const int indicator_position =  6; /* start and end of indicator area */
const int a_margin           =  7; /* start of A margin */
const int comment_area       = 72; /* start of comment area */

/* The comment area is not stored in the card variable. Thus: */
#define CARD_SIZE 73

char *program;         /* argv[0] */
char card[CARD_SIZE];  /* a null-terminated card from the program's deck */
bool has_comment_area; /* does the current card have a comment area? */
char ch;               /* the last character read */

bool get_card();

bool is_comment_line();
bool is_comment_par();

void print_card();
void print_comment_par();
void print_comment_line();
void print_code_line();
void print_comment_area();
void print_linebreaks();

int  ascii_strnicmp(const char *a, const char *b, size_t count);

int main(int argc, char *argv[])
{
    program = argv[0];
    ch = '\0';

    while (get_card())
        print_card();
}

/* get_card: Get next card from standard input. */
bool get_card()
{
    int i;

    /* Reset the card. */
    for (i = 0; i < CARD_SIZE; ++i)
        if (card[i])
            card[i] = '\0';
        else
            break;
    has_comment_area = false;

    i = 0;
    if (ch)
        card[i++] = ch; /* Start the card with the last character read. */

    /* Read each card position until comment area, linebreak, or EOF. */
    while ((ch = getchar()) != EOF) {
        if (ch == '\r' || ch == '\n')
            break;
        card[i++] = ch;
        if (i == comment_area) {
            has_comment_area = true;
            break;
        }
    }

    return (i > 0 || ch != EOF);
}

/* print_card: Print the current card. */
void print_card()
{
    char indicator;

    /* Print the sequence area verbatim. */
    if (card[sequence_area])
        for (int i = sequence_area; i < indicator_position && card[i]; ++i)
            putchar(card[i]);
    else
        return;

    /* Print the indicator area in lowercase. */
    if (card[indicator_position])
        putchar(tolower(card[indicator_position]));
    else
        return;

    if (card[a_margin])
        if (is_comment_line())
            print_comment_line();
        else if (is_comment_par())
            print_comment_par();
        else
            print_code_line();
    else
        return;

    if (has_comment_area)
        print_comment_area();

    print_linebreaks();
}

/* is_comment_line: Is the card a comment line? */
bool is_comment_line()
{
    char i = card[indicator_position];
    return (i == '*' || i == '/' || i == '$');
}

/* is_comment_par: Is the card a documentation-only paragraph? */
bool is_comment_par()
{
    static const char* const par[] = {
        "AUTHOR.",
        "INSTALLATION.",
        "DATE-WRITTEN.",
        "DATE-COMPILED.",
        "SECURITY.",
        "REMARKS."
    };
    static const int len[] = { 7, 13, 13, 14, 9, 8 };
    static const int size = sizeof par / sizeof(char*);

    for (int i = 0; i < size; ++i)
        if (strlen(card + a_margin) >= len[i])
            if (ascii_strnicmp(par[i], card + a_margin, len[i]) == 0)
                return true;
    return false;
}

/* print_comment_line: Print the A and B margins verbatim. */
void print_comment_line()
{
    for (int i = a_margin; i < comment_area && card[i]; ++i)
        putchar(card[i]);
}

/* print_comment_par: Print the A + B margins as a comment paragraph.   */
/*                    Assumption: Columns 1-7 have already been printed. */
void print_comment_par()
{
    int i;

    /* Print the paragraph name in lowercase. */
    for (i = a_margin; card[i] != '.'; ++i)
         putchar(tolower(card[i]));

    /* Print the rest of the line verbatim. */
    printf("%s", card + i);
}

/* print_code_line: Print the A + B margins with normal formatting.   */
/*                  Assumption: Columns 1-7 have already been printed. */
void print_code_line()
{
    char quote;
    enum contexts context = CODE;

    for (int i = a_margin; i < comment_area && card[i]; ++i)
        switch (context) {
            case CODE:
                putchar(tolower(card[i]));
                if (card[i] == '"' || card[i] == '\'') {
                    context = LITERAL;
                    quote = card[i];
                } else if (card[i - 1] == '=' && card[i] == '=')
                    context = PSEUDOTEXT;
                break;
            case LITERAL:
                putchar(card[i]);
                if (card[i] == quote)
                    context = CODE;
                break;
            case PSEUDOTEXT:
                putchar(card[i]);
                if (card[i] == '=' && card[i + 1] == '=')
                    context = CODE;
                break;
        }
}

/* print_comment_area: Print all text verbatim through end of line or EOF. */
/*                     Assumption: Columns 1-72 have already been printed. */
/*                     Fixed 80-column line length is not enforced.        */
void print_comment_area()
{
    while ((ch = getchar()) != EOF) {
        if (ch == '\r' || ch == '\n')
            break;
        putchar(ch);
    }
}

/* print_code_line: Print linebreaks until next non-blank line.   */
/*                  Assumption: Last character read was \r or \n. */
void print_linebreaks()
{
    switch (ch) {
        case '\r':
            putchar(ch);
            if ((ch = getchar()) == '\n')
                putchar('\n');
            else {
                fprintf(stderr, "%s: bad linebreak (CR without LF)\n", program);
                exit (CR_WITHOUT_LF);
            }
            ch = '\0';
            break;
        case '\n':
            putchar(ch);
            ch = '\0';
            break;
    }
}

/* ascii_strnicmp: Case-insensitive ASCII string comparison. */
int ascii_strnicmp(const char *a, const char *b, size_t count)
{
    int diff = 0;

    do {
        diff = (toupper(*a++) - toupper(*b++));
    } while (!diff && *a && --count);

    return diff;
}