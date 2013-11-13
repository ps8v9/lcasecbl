/*
 * lcasecbl.c : C99 utility for formatting COBOL in lowercase.
 * AUTHOR     : Matthew J. Fisher
 * REPO       : https://github.com/ps8v9/lcasecbl
 * LICENSE    : Public domain. See LICENSE file for details.
 */
#include <stdio.h>

enum indicator {
    SPACE,
    HYPHEN,
    ASTERISK,
    DOLLAR_SIGN,
    SLASH,
    D
};

int main()
{
    int c;
    while ((c = getchar()) != EOF) {
        // Obviously there is still some work to do here.
        putchar(tolower(c));
    }
}