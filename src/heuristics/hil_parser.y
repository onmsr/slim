/*
 * heuristic search LMNtal -- parser
 */

/*
  lines ::= line { line }
  line ::= instruction '[' args ']' dest | label
  label ::= identifier ":"
  args ::= arg { ',' arg }
  arg ::= identifier
  instruction ::= identifier
  dest ::= identifier
  identifier ::= [a-zA-Z_0-9]*
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
    void label_push(HLabel label);
    extern char *hiltext;

#define COPY_TEXT(text)                      \ 
  {                                                      \ 
    hiltext = text;                    \ 
  }

  HLabelList labels;
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
%type <_string> dest
%type <_id> instruction
%type <_instr> line
%type <_instrs> lines
%type <_string> label

%parse-param {HIL *ret_hil}

%start root

%%

root : lines { *ret_hil = hil_make($1, labels); }

lines :
line { HInstrList l = hinstr_list_make(); if($1){ hinstr_list_push(l, $1); } else { HLabel label = hlabel_make((HLabelStr) hiltext, l->num); label_push(label); } $$ = l; }
| lines line {  if($2){ hinstr_list_push($1, $2); } else { HLabel label = hlabel_make((HLabelStr) hiltext, $1->num); label_push(label); } $$ = $1; }
;

line :
  instruction '[' args ']' dest  { $$ = hinstr_make($1, $3, (HDest) $5); }
| label { COPY_TEXT($1); $$ = (HInstruction) NULL; }
;

label : IDENTIFIER ':' { $$ = $1; }

instruction : IDENTIFIER { $$ = get_hinstr_id($1); }

dest : IDENTIFIER { $$ = $1; }

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

void label_push(HLabel label)
{
  if (!labels) {
    labels = hlabel_list_make();
  }
  hlabel_list_push(labels, label);
}
