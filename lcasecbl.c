#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum areas {
    NO_AREAS,
    SEQ_AREA     =  1,
    IND_AREA     =  2,
    A_MARGIN     =  4,
    B_MARGIN     =  8,
    COMMENT_AREA = 16
};

enum contexts {
    CODE,
    LITERAL,
    PSEUDOTEXT
};

enum errors {
    CR_WITHOUT_LF = 1
};

const int seq_area     =  0; /* start of sequence area */
const int ind_area     =  6; /* start and end of indicator area */
const int a_margin     =  7; /* start of A margin */
const int comment_area = 72; /* start of comment area */

/* The comment area is not stored in the card variable. Thus: */
#define CARD_SIZE 73

const char* program;   /* argv[0] */
char card[CARD_SIZE];  /* a null-terminated card from the program's deck */
bool has_comment_area; /* does the current card have a comment area? */
char ch;               /* the last character read */
int areas_printed;     /* which areas have been printed from the card? */

bool get_card();
bool is_comment_line();
bool is_comment_par();
bool is_normal_line();
bool is_debugging_line();
bool is_continuation_line();

void print_card();
void print_seq_area();
void print_ind_area();
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
    areas_printed = NO_AREAS;

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
    /* Print the sequence area verbatim. */
    if (card[seq_area])
        print_seq_area();
    else
        return;

    /* Print the indicator area in lowercase. */
    if (card[ind_area])
        print_ind_area();
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
    char i = card[ind_area];
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

/* is_normal_line: Is the card a normal line? */
bool is_normal_line() {
    return (card[ind_area] == ' ');
}

/* is_debugging_line: Is the card a debugging line? */
bool is_debugging_line()
{
    return (card[ind_area] == '/');
}

/* is_continuation_line: Is the card a continuation line? */
bool is_continuation_line()
{
    return (card[ind_area] == '-');
}

/* print_seq_area: Print the sequence area verbatim. */
void print_seq_area()
{
    assert(areas_printed == NO_AREAS);

    for (int i = seq_area; i < ind_area && card[i]; ++i)
        putchar(card[i]);

    areas_printed |= SEQ_AREA;
}

/* print_indicator_area(): Print the indicator area in lowercase. */
void print_ind_area()
{
    assert(areas_printed == SEQ_AREA);

    putchar(tolower(card[ind_area]));

    areas_printed |= IND_AREA;
}

/* print_comment_line: Print the A and B margins verbatim. */
void print_comment_line()
{
    assert(is_comment_line());
    assert(areas_printed == SEQ_AREA | IND_AREA);

    for (int i = a_margin; i < comment_area && card[i]; ++i)
        putchar(card[i]);

    areas_printed |= A_MARGIN | B_MARGIN;
}

/* print_comment_par: Print the A + B margins as a comment paragraph.   */
void print_comment_par()
{
    assert(is_comment_par());
    assert(areas_printed == SEQ_AREA | IND_AREA);

    int i;

    /* Print the paragraph name in lowercase. */
    for (i = a_margin; card[i] != '.'; ++i)
         putchar(tolower(card[i]));

    /* Print the rest of the line verbatim. */
    printf("%s", card + i);

    areas_printed |= A_MARGIN | B_MARGIN;
}

/* print_code_line: Print the A + B margins with normal formatting. */
void print_code_line()
{
    assert(is_normal_line() || is_continuation_line() || is_debugging_line());
    assert(areas_printed == SEQ_AREA | IND_AREA);

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

    areas_printed |= A_MARGIN | B_MARGIN;
}

/* print_comment_area: Print comment area verbatim until end of line or EOF. */
/*                     Fixed 80-column line length is not enforced.          */
void print_comment_area()
{
    assert(areas_printed == SEQ_AREA | IND_AREA | A_MARGIN | B_MARGIN);

    while ((ch = getchar()) != EOF) {
        if (ch == '\r' || ch == '\n')
            break;
        putchar(ch);
    }

    areas_printed |= A_MARGIN | B_MARGIN;
}

/* print_code_line: Print linebreaks until next non-blank line.   */
void print_linebreaks()
{
    assert(ch == '\r' || ch == '\n');

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