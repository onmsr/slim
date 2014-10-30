/*
 *    heuristic search LMNtal -- lexical analysis
 */

%{
  /* c header part */

#include "hsyntax.h"
#include "hil_parser.h"
#include <stdio.h>
#include <string.h>

#define COPY_AND_RETURN(token_type)                      \ 
  {                                                      \ 
    hillval._string = strdup(yytext);                      \ 
    return (token_type);                                 \ 
  }
  
%}

%s COMMENT
COMIN "/*"
COMOUT "*/"

%s LIST
LISTIN "["
LISTOUT "]"

DIGIT               [0-9]
NUMBER              {DIGIT}+
LETTER              [a-zA-Z_]
LITERAL             {LETTER}}+
LETTER_OR_DIGIT     {LETTER}|{DIGIT}
WORD                {LETTER_OR_DIGIT}+
WHITE_SPACE         [ \t]
NEWLINE             [\n|\r|\r\n]

 /* formula */
%%

<INITIAL>{COMIN}      { printf("begin comment!\n"); BEGIN COMMENT; }
<COMMENT>{COMOUT}     { printf("end comment!\n"); BEGIN INITIAL; }
<COMMENT>.            { printf("%s", yytext); }

<INITIAL>{LISTIN}     { printf("begin list!\n["); BEGIN LIST; return yytext[0]; }
<LIST>{LISTOUT}       { printf("]\nend list!\n"); BEGIN INITIAL; return yytext[0]; }
<LIST>{WHITE_SPACE}   { /* skip white space */ }
<LIST>{WORD}          { printf("%d: list word  : %s\n", yylineno, yytext); COPY_AND_RETURN(LIST_IDENTIFIER); }
<LIST>.               { printf("%d: list other : %s\n", yylineno, yytext); return yytext[0]; }

<INITIAL>{WHITE_SPACE}    { /* skip white space */ }
<INITIAL>{NEWLINE}        { yylineno++; }
<INITIAL>{WORD}           { printf("%d: word  : %s\n", yylineno, yytext); COPY_AND_RETURN(IDENTIFIER); }
<INITIAL>.                { printf("%d: other : %s\n", yylineno, yytext); return yytext[0]; }

%%

