lcasecbl
========

C utility to format COBOL in lowercase.

Comment lines, alphanumeric literals, hex literals, pseudo-text, the sequence
area, the indicator area, and the comment area are not changed.

Everything else is converted to lowercase. This is not necessarily valid for
all COBOL dialects.

ASSUMPTIONS
===========

The COBOL source code is assumed to be in ANSI format (a.k.a. reference format).

The code is assumed to compile, so that unexpected conditions like unterminated
alphanumeric literals will not occur.

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

CAVEATS
=======

Areas where I suspect that breakage may occur in at least some dialects of
COBOL are:

    case-sensitive picture clauses