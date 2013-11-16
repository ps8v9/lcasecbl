#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* These magic numbers will never change. Fixed format is fixed forever. */
#define CARD_SIZE 73
const int seq_area     =  0; /* start of sequence area */
const int ind_area     =  6; /* start and end of indicator area */
const int a_margin     =  7; /* start of A margin */
const int comment_area = 72; /* start of comment area */

/* Columns 73+ are not ignored. They're just not stored in this struct. */
struct card_format {
    char data[CARD_SIZE];  /* a null-terminated card from the program's deck */
    bool eof;              /* did we reach eof before next card? */
    bool is_blank;         /* is the card blank? */
    bool has_comment_area; /* does the card have a comment area? */
    int  areas_printed;    /* which areas have been printed from the card? */
};

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

void read_card();

bool is_comment_line();
bool is_comment_par();
bool is_normal_line();
bool is_continuation_line();
bool is_debugging_line();

void print_card();
void print_seq_area();
void print_ind_area();
void print_comment_par();
void print_comment_line();
void print_code_line();

void echo_comment_area();
void echo_linebreaks();

int ps8_strnicmp(const char *a, const char *b, size_t count);

const char* program;
struct card_format card;

int main(int argc, char *argv[])
{
    program = argv[0];

    while (! card.eof) {
        read_card();
        if (! card.is_blank)
            print_card();
        if (card.has_comment_area)
            echo_comment_area();
        echo_linebreaks();
    }

    return 0;
}

/* read_card: Read next card from stdin. */
void read_card()
{
    int ch;
    int i;

    /* Reset the card. */
    for (i = 0; i < CARD_SIZE; ++i)
        if (card.data[i])
            card.data[i] = '\0';
        else
            break;
    card.is_blank = true;
    card.has_comment_area = false;
    card.areas_printed = NO_AREAS;
    i = 0;

    /* Read each card position until comment area, linebreak, or EOF. */
    while ((ch = getchar()) != EOF) {
        if (ch == '\r' || ch == '\n') {
            ungetc(ch, stdin);
            break;
        }
        card.data[i++] = ch;
        if (i == comment_area) {
            card.has_comment_area = true;
            break;
        }
    }

    card.is_blank = (i == 0);
    card.eof = (ch == EOF);
}

/* print_card: Print the current card. */
void print_card()
{
    if (card.data[seq_area])
        print_seq_area();
    else
        return;

    if (card.data[ind_area])
        print_ind_area();
    else
        return;

    if (card.data[a_margin])
        if (is_comment_line())
            print_comment_line();
        else if (is_comment_par())
            print_comment_par();
        else
            print_code_line();
    else
        return;

    if (card.has_comment_area)
        echo_comment_area();
}

/* is_comment_line: Is the card a comment line? */
bool is_comment_line()
{
    char ind = card.data[ind_area];
    return (ind == '*' || ind == '/' || ind == '$');
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
        if (strlen(card.data + a_margin) >= len[i])
            if (ps8_strnicmp(par[i], card.data + a_margin, len[i]) == 0)
                return true;
    return false;
}

/* is_normal_line: Is the card a normal line? */
bool is_normal_line() {
    return (card.data[ind_area] == ' ');
}

/* is_debugging_line: Is the card a debugging line? */
bool is_debugging_line()
{
    return (card.data[ind_area] == '/');
}

/* is_continuation_line: Is the card a continuation line? */
bool is_continuation_line()
{
    return (card.data[ind_area] == '-');
}

/* print_seq_area: Print the card's sequence area verbatim. */
void print_seq_area()
{
    assert(card.areas_printed == NO_AREAS);
    for (int i = seq_area; i < ind_area && card.data[i]; ++i)
        putchar(card.data[i]);
    card.areas_printed |= SEQ_AREA;
}

/* print_indicator_area(): Print the card's indicator area in lowercase. */
void print_ind_area()
{
    assert(card.areas_printed == SEQ_AREA);
    putchar(tolower(card.data[ind_area]));
    card.areas_printed |= IND_AREA;
}

/* print_comment_line: Print the card's A and B margins verbatim. */
void print_comment_line()
{
    assert(is_comment_line());
    assert(card.areas_printed == (SEQ_AREA | IND_AREA));
    for (int i = a_margin; i < comment_area && card.data[i]; ++i)
        putchar(card.data[i]);
    card.areas_printed |= A_MARGIN | B_MARGIN;
}

/* print_comment_par: Print the card's A + B margins as a comment paragraph. */
void print_comment_par()
{
    assert(is_comment_par());
    assert(card.areas_printed == (SEQ_AREA | IND_AREA));

    int i;

    /* Print the paragraph name in lowercase. */
    for (i = a_margin; card.data[i] != '.'; ++i)
         putchar(tolower(card.data[i]));

    /* Print the rest of the line verbatim. */
    printf("%s", card.data + i);

    card.areas_printed |= A_MARGIN | B_MARGIN;
}

/* print_code_line: Print the card's A + B margins with normal formatting. */
void print_code_line()
{
    assert(is_normal_line() || is_continuation_line() || is_debugging_line());
    assert(card.areas_printed == (SEQ_AREA | IND_AREA));

    char quote;
    enum contexts context = CODE;

    for (int i = a_margin; i < comment_area && card.data[i]; ++i)
        switch (context) {
            case CODE:
                putchar(tolower(card.data[i]));
                if (card.data[i] == '"' || card.data[i] == '\'') {
                    context = LITERAL;
                    quote = card.data[i];
                } else if (card.data[i - 1] == '=' && card.data[i] == '=')
                    context = PSEUDOTEXT;
                break;
            case LITERAL:
                putchar(card.data[i]);
                if (card.data[i] == quote)
                    context = CODE;
                break;
            case PSEUDOTEXT:
                putchar(card.data[i]);
                if (card.data[i] == '=' && card.data[i + 1] == '=')
                    context = CODE;
                break;
        }

    card.areas_printed |= A_MARGIN | B_MARGIN;
}

/* echo_comment_area: Read and print verbatim from the comment area until end   */
/*                    of line or EOF. Fixed 80-col line length is not enforced. */
void echo_comment_area()
{
    assert(card.areas_printed == (SEQ_AREA | IND_AREA | A_MARGIN | B_MARGIN));

    int ch;

    while ((ch = getchar()) != EOF) {
        if (ch == '\r' || ch == '\n') {
            ungetc(ch, stdin);
            break;
        }
        putchar(ch);
    }

    card.eof = (ch == EOF);
    card.areas_printed |= A_MARGIN | B_MARGIN;
}

/* echo_linebreaks: Read and print linebreaks until non-blank line or EOF. */
void echo_linebreaks()
{
    bool done;
    char ch;

    while (! done) {
        switch (ch = getchar()) {
            case '\r':
                putchar(ch);
                if ((ch = getchar()) == '\n')
                    putchar('\n');
                else {
                    fprintf(stderr, "%s: bad input (CR without LF)\n", program);
                    exit(CR_WITHOUT_LF);
                }
                break;
            case '\n':
                putchar(ch);
                break;
            case EOF:
                card.eof = true;
                done = true;
                break;
            default:
                ungetc(ch, stdin);
                done = true;
                break;
        }
        if (done)
            break;
    }
}

/* ps8_strnicmp: Case-insensitive string comparison. */
int ps8_strnicmp(const char *a, const char *b, size_t count)
{
    int diff = 0;

    do {
        diff = (toupper(*a++) - toupper(*b++));
    } while (!diff && *a && --count);

    return diff;
}