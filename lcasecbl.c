#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* These magic numbers will never change. Fixed format is fixed forever. */
#define CARD_SIZE 73
const int seq_area     =  0; /* start of sequence area          */
const int ind_area     =  6; /* start and end of indicator area */
const int a_margin     =  7; /* start of A margin               */
const int comment_area = 72; /* start of comment area           */

/* Columns 73+ are not ignored. They're just not stored in this struct. */
struct card_format {
    char data[CARD_SIZE];      /* a null-terminated card from the deck  */
    bool eof;                  /* Did we reach EOF before next card?    */
    bool has_data;             /* Does the card have data in cols 1-72? */
    bool has_comment_area;     /* Does it have a comment area?          */
    bool is_normal_line;       /* Is the card a normal line?            */
    bool is_continuation_line; /*         ... a continuation line?      */
    bool is_comment_line;      /*         ... a comment line?           */
    bool is_debugging_line;    /*         ... a debugging line?         */
    bool is_comment_par;       /*         ... a comment paragraph?      */
    int  areas_printed;        /* Which of its areas have been printed? */
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
void set_properties(int cnt, bool eof);

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
        if (card.has_data)
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
    int i;
    int ch;

    /* Reset the card's data member. */
    for (i = 0; i < CARD_SIZE; ++i)
        if (card.data[i])
            card.data[i] = '\0';
        else
            break;

    /* Read each card position until comment area, linebreak, or EOF. */
    i = 0;
    while ((ch = getchar()) != EOF) {
        if (ch == '\r' || ch == '\n') {
            ungetc(ch, stdin);
            break;
        }
        card.data[i++] = ch;
        if (i == comment_area) {
            break;
        }
    }

    set_properties(i, (ch == EOF));
}

/* set_properties: Set the card's properties. */
void set_properties(int cnt, bool eof)
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

    card.eof = eof;
    card.has_data = (cnt > 0);
    card.has_comment_area = (cnt == comment_area);

    char ind = card.data[ind_area];
    card.is_normal_line = (cnt> 0 && ind==' ');
    card.is_continuation_line = (cnt>0 && ind=='-');
    card.is_comment_line = (cnt>0 && (ind=='*' || ind=='/' || ind=='$'));
    card.is_debugging_line = (cnt>0 && ind=='/');

    card.is_comment_par = false; /* default */
    for (int i = 0; i < size; ++i)
        if (strlen(card.data + a_margin) >= len[i])
            if (ps8_strnicmp(par[i], card.data + a_margin, len[i]) == 0)
                card.is_comment_par = true;

    card.areas_printed = NO_AREAS;
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
        if (card.is_comment_line)
            print_comment_line();
        else if (card.is_comment_par)
            print_comment_par();
        else
            print_code_line();
    else
        return;

    if (card.has_comment_area)
        echo_comment_area();
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
    assert(card.is_comment_line);
    assert(card.areas_printed == (SEQ_AREA | IND_AREA));
    for (int i = a_margin; i < comment_area && card.data[i]; ++i)
        putchar(card.data[i]);
    card.areas_printed |= A_MARGIN | B_MARGIN;
}

/* print_comment_par: Print the card's A + B margins as a comment paragraph. */
void print_comment_par()
{
    assert(card.is_comment_par);
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
    assert(card.is_normal_line       ||
           card.is_continuation_line ||
           card.is_debugging_line);
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