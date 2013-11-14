#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

enum errors { ERR_CR_WITHOUT_LF = 1 };

const int sequence_area      =  0; /* start of sequence area */
const int indicator_position =  6; /* start and end of indicator area */
const int a_margin           =  7; /* start of A margin */
const int comment_area       = 72; /* start of comment area */

/* The comment area is not stored in the card variable. Thus: */
#define CARD_SIZE 73

char *program;         /* argv[0] */
char card[CARD_SIZE];  /* one card from the program's deck */
bool has_comment_area; /* current card has a comment area */
char ch;               /* last character read */

int get_card();
void print_card();
void print_comment_area();
void print_linebreaks();

int main(int argc, char *argv[])
{
    program = argv[0];
    ch = '\0';

    while (ch != EOF) {
        if (!get_card())
            continue; /* Card is empty. */

        print_card();

        if (has_comment_area)
            print_comment_area();

        print_linebreaks();
    }
}

int get_card()
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

    return i;
}

void print_card()
{
    char indicator;
    char quote;
    bool in_literal     = false;
    bool in_pseudo_text = false;

    /* Print the sequence area verbatim. */
    if (card[sequence_area])
        for (int i = sequence_area; i < indicator_position && card[i]; ++i)
            putchar(card[i]);
    else
        return;

    /* Print the indicator area as lowercase. */
    if ((indicator = card[indicator_position]))
        putchar(tolower(indicator));
    else
        return;

    if (indicator == '*' || indicator == '/' || indicator == '$') {
        /* This is a comment line. Print the A and B margins verbatim. */
        if (card[a_margin])
            for (int i = a_margin; i < comment_area && card[i]; ++i)
                putchar(card[i]);
    } else {
         /*
          * Print the A and B margins as lowercase, with the exception of
          * literals and pseudo-text.
          */
         if (card[a_margin])
            for (int i = a_margin; i < comment_area && card[i]; ++i)
                if (!in_literal && !in_pseudo_text) {
                    putchar(tolower(card[i]));
                    if (card[i] == '"' || card[i] == '\'') {
                        in_literal = true;
                        quote = card[i];
                    } else if (card[i - 1] == '=' && card[i] == '=')
                        in_pseudo_text = true;
                } else if (in_literal) {
                    putchar(card[i]);
                    if (card[i] == quote)
                        in_literal = false;
                } else if (in_pseudo_text) {
                    putchar(card[i]);
                    if (card[i] == '=')
                        in_pseudo_text = false;
                }
    }
}

void print_comment_area()
{
    while ((ch = getchar()) != EOF) {
        if (ch == '\r' || ch == '\n')
            break;
        putchar(ch);
    }
}

void print_linebreaks()
{
    switch (ch) {
        case '\r':
            putchar(ch);
            if ((ch = getchar()) == '\n')
                putchar('\n');
            else {
                fprintf(stderr, "%s: bad linebreak (CR without LF)\n", program);
                exit (ERR_CR_WITHOUT_LF);
            }
            ch = '\0';
            break;
        case '\n':
            putchar(ch);
            ch = '\0';
            break;
    }
}
