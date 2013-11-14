lcasecbl
========

C utility to format COBOL in lowercase.

Comment lines, alphanumeric literals, hex literals, pseudo-text, the sequence
area, the indicator area, and the comment area are not changed. The comment
area is considered to extend from column 73 through the end of the line.

Everything else is converted to lowercase. This is not necessarily valid for
all COBOL dialects.

ASSUMPTIONS
===========

The COBOL source code is assumed to be in ANSI format (a.k.a. reference format).
The only exception is that the comment area is allowed to be arbitrarily long.
This is done to prevent truncation of overlong comments.

The code is assumed to compile, so that unexpected conditions like unterminated
alphanumeric literals and empty pseudo-text will not occur. Continuation lines
are supported.

Alphanumeric and hex literals are assumed to be delimited in the following ways:

    "abcdef"
    'abcdef'

Pseudo-text is assumed to be delimited as follows:

    ==some text goes here==

Alphanumeric literals, hex literals, and pseudo-text may continue across line
breaks. Continuation lines have a hyphen in the indicator area.

Normal lines have a space in the indicator area.

Conditional debugging lines have a 'D' or 'd' in the indicator area, and are
treated like normal lines.

Comment lines have one of the following in the indicator area: an asterisk (*),
a slash (/), or a dollar sign ($).

Lines may continue beyond the comment area, but anything past column 80 is
considered to be a comment.

CAVEATS
=======

Areas where I suspect that breakage may occur in at least some dialects of
COBOL are:

    case-sensitive picture clauses