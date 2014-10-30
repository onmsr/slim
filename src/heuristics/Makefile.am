noinst_LIBRARIES = liblmn_heuristics.a

AM_LFLAGS=-P $(patsubst %_lexer.l,%,$<) --header-file=$(patsubst %.l,%.h,$<) -o $(patsubst %.l,%.c,$<)
AM_YFLAGS=-d -p $(patsubst %_parser.y,%,$<) -o $(patsubst %.y,%.c,$<)

PARSERS = hil_parser hil_lexer
PARSER_HEADER = $(patsubst %, %.h, $(PARSERS))
PARSER_SOURCE = $(patsubst %, %.c, $(PARSERS))

BUILT_SOURCES = $(PARSER_SOURCE)
CLEANFILES = $(PARSER_SOURCE) $(PARSER_HEADER)

liblmn_heuristics_a_CFLAGS = \
        -I../                \
        -I../utility         \
        -I../verifier        \
        $(CFLAGS)

liblmn_heuristics_a_SOURCES =       \
	hil_parser.y   hil_lexer.l        \
	hsyntax.c      hsyntax.h          \
	heuristics.c   heuristics.h

EXTRA_DIST = $(PARSER_SOURCE) $(PARSER_HEADER)

.l.c:
	$(LEX) $(LFLAGS) $(AM_LFLAGS) $<
.y.c:
	$(YACC) $(YFLAGS) $(AM_YFLAGS) $<
