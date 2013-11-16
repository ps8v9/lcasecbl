/*
 * lcasecbl.c : C99 utility to format COBOL in lowercase
 *
 * AUTHOR  : Matthew J. Fisher
 * REPO    : https://github.com/ps8v9/lcasecbl
 * LICENSE : This is free and unencumbered software released into the public
 *           domain. See the LICENSE file for further details.
 */
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

/* Columns > 72 are not ignored. They're just not stored in this struct. */
struct {
    char data[CARD_SIZE];  /* a null-terminated card from the deck  */
    bool eof;              /* Did we reach EOF before next card? */
    bool has_data;         /* Does the card have data in cols 1-72? */
    bool has_comment_area; /* Does it have a comment area? */
    bool is_comment_line;  /* Is the card a comment line? */
    bool is_comment_par;   /* Is it a comment paragraph? */
} card;

struct { bool tolower; } opts;

enum contexts { CODE, LITERAL, PSEUDOTEXT };

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
int  ps8_strnicmp(const char *a, const char *b, size_t count);
int  ps8_getopt();
char to_target_case(const char ch);

int main(int argc, char *argv[])
{
    int err_code = ps8_getopt(argc, argv);

    if (err_code != 0)
        exit(err_code);

    while (! card.eof) {
        read_card();
        if (card.has_data)
            print_card();
        echo_linebreaks();
    }

    return 0;
}

/* read_card: Read next card from stdin. */
void read_card()
{
    int i, ch;

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
        card.data[i++] = (char)ch;
        if (i == comment_area)
            break;
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
    const char ind = card.data[ind_area];

    card.eof = eof;
    card.has_data = (cnt > 0);
    card.has_comment_area = (cnt == comment_area);
    card.is_comment_line = (cnt>0 && (ind=='*' || ind=='/' || ind=='$'));

    card.is_comment_par = false; /* default */
    for (int i = 0; i < size; ++i)
        if (strlen(card.data + a_margin) >= len[i])
            if (ps8_strnicmp(card.data + a_margin, par[i], len[i]) == 0)
                card.is_comment_par = true;
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
    for (int i = seq_area; i < ind_area && card.data[i]; ++i)
        putchar(card.data[i]);
}

/* print_indicator_area(): Print the card's indicator area in target case. */
void print_ind_area()
{
    putchar(to_target_case(card.data[ind_area]));
}

/* print_comment_line: Print the card's A and B margins verbatim. */
void print_comment_line()
{
    for (int i = a_margin; i < comment_area && card.data[i]; ++i)
        putchar(card.data[i]);
}

/* print_comment_par: Print the card's A + B margins as a comment paragraph. */
void print_comment_par()
{
    int i;

    /* Print the paragraph name in lowercase. */
    for (i = a_margin; card.data[i] != '.'; ++i)
         putchar(to_target_case(card.data[i]));

    /* Print the rest of the line verbatim. */
    printf("%s", card.data + i);
}

/* print_code_line: Print the card's A + B margins with normal formatting. */
void print_code_line()
{
    char quote;
    enum contexts context = CODE;

    for (int i = a_margin; i < comment_area && card.data[i]; ++i)
        switch (context) {
            case CODE:
                putchar(to_target_case(card.data[i]));
                if (card.data[i] == '"' || card.data[i] == '\'') {
                    quote = card.data[i];
                    context = LITERAL;
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
                if (card.data[i - 1] == '=' && card.data[i] == '=')
                    context = CODE;
                break;
        }
}

/* echo_comment_area: Read and print verbatim from the comment area until end   */
/*                    of line or EOF. Fixed 80-col line length is not enforced. */
void echo_comment_area()
{
    int ch;

    while ((ch = getchar()) != EOF) {
        if (ch == '\r' || ch == '\n') {
            ungetc(ch, stdin);
            break;
        }
        putchar(ch);
    }

    card.eof = (ch == EOF);
}

/* echo_linebreaks: Read and print linebreaks until non-blank line or EOF. */
void echo_linebreaks()
{
    int  ch;
    bool done = false;

    while (! done) {
        switch (ch = getchar()) {
            case '\r':
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

/* ps8_getopt: Poor man's getopt. Supported options are -h to show the usage   */
/*             statement, -l to convert to lowercase, and -L to convert to     */
/*             uppercase. Options may start with - or /.                       */
int ps8_getopt(int argc, char *argv[])
{
    int err_code = 0;

    if (argc > 0 && (argv[1][0] == '-' || argv[1][0] == '/'))
        switch (argv[1][1]) {
            case 'l':
                opts.tolower = true;
                break;
            case 'L':
                opts.tolower = false;
                break;
            case 'h':
                fprintf(stderr, "usage: %s %c[hlL]\n", argv[0], argv[1][0]);
                err_code = 1;
                break;
            default:
                fprintf(stderr, "%s: unknown option: %c%c\n", argv[0],
                  argv[1][0], argv[1][1]);
                err_code = 2;
                break;
        }

    return err_code;
}

/* to_target_case: Convert to lower or uppercase according to option setting. */
char to_target_case(const char ch)
{
    return (opts.tolower) ? tolower(ch) : toupper(ch);
}