lcasecbl
========

C99 utility to convert COBOL to lowercase or uppercase.

The following are excluded from being converted to the target case:

    comment lines
    sequence area  (columns 1-6)
    indicator area (column 7)
    comment area   (column 73 through end of line)
    alphanumeric literals
    hexadecimal literals
    pseudo-text

Everything else is converted to the target case. This is not necessarily valid.

Options
===========

A few options are supported. Options may start with either a hyphen (-) or a slash (/).

    -h  Help.       Show the usage statement.
    -u  Uppercase.  Convert code to uppercase rather than to lowercase.

Assumptions
===========

The COBOL source code is assumed to be in ANSI format (a.k.a. reference format).
The only exception is that the comment area is allowed to be arbitrarily long.
This is done to prevent truncation of overlong comments.

The code is assumed to compile, so that unexpected conditions like unterminated
alphanumeric literals and unterminated pseudo-text will not occur. Continuation
lines are supported.

Alphanumeric and hex literals are assumed to be delimited in the following ways:

    "abcdef"
    'abcdef'

Pseudo-text is assumed to be delimited as follows:

    ==some text goes here==

Alphanumeric literals, hex literals, and pseudo-text may continue across line
breaks. Continuation lines have a hyphen in the indicator area.

Normal lines have a space in the indicator area.

Conditional debugging lines have a 'D' or 'd' in the indicator area, and are
treated here like normal lines.

Comment lines have one of the following in the indicator area: an asterisk (*),
a slash (/), or a dollar sign ($).

Lines may continue beyond the comment area, which starts in column 73 and ends
in column 80. Everything past column 72 is considered to be a comment.

Caveats
=======

Areas where I suspect that breakage may occur in at least some dialects of
COBOL are:

    case-sensitive picture clauses
    space-delimited literals
