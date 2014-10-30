/*
 * heuristic search LMNtal -- parser
 */

/*
  inst term*
  term ::= args | arg
  args ::= '[' arg * ']'
  arg ::= [a-z][a-z]* | 'T'[0-9][0-9]*
  'call' funcname args arg
*/

%{
  /* c header part */
  
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "hsyntax.h"
#define YYDEBUG 1
    int hilwrap();
    int hilerror(HIL *hil, char const *str);
    extern int hillex();
%}

%union {
  char *_string;
  HArgument _arg;
  HArgList _args;
  enum LmnHInstruction _id;
  HInstruction _instr;
  HInstrList _instrs;
}

%token <_string> IDENTIFIER
%token <_string> LIST_IDENTIFIER

%type <_arg> arg
%type <_args> args
%type <_string> dist
%type <_id> instruction
%type <_instr> line
%type <_instrs> lines

%parse-param {HIL *ret_hil}

%start root

%%

root : lines { *ret_hil = hil_make($1); }

lines :
  line { HInstrList l = hinstr_list_make(); hinstr_list_push(l, $1); $$ = l; }
| lines line { hinstr_list_push($1, $2); $$ = $1; }
;

line : instruction '[' args ']' dist  { $$ = hinstr_make($1, $3, (HDest) $5); }

instruction : IDENTIFIER { $$ = get_hinstr_id($1); }

dist : IDENTIFIER { $$ = $1; }

args:
  arg  { HArgList l = harg_list_make(); harg_list_push(l, $1); $$ = l; }
| args ',' arg { harg_list_push($1, $3); $$ = $1; }
;

arg : LIST_IDENTIFIER { $$ = harg_make((HArg) $1); }

%%

int hilwrap(void)
{
    return 1;
}

int hilerror(HIL *hil, char const *str)
{
    extern char *hiltext;
    fprintf(stderr, "hil parser error near %s\n", hiltext);
    return 0;
}

