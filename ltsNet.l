%option noyywrap yylineno

%x comment

%{
#include <stdio.h>
#include <string.h>
#include "automata_interface.h"
#include "ltsNet.tab.h"
#include "common.h"
%}

/* abbrev defns */
ALPHASTRING ([a-zA-Z0-9\@\_]+)
WSPACE ([ \t\n]*)

/* reserved words */
STATES "states"
TRANSITIONS "transitions"

/* lexer rules */
%%

[ \t\n\r]+ { }

{STATES} { return STATES; }
{TRANSITIONS} { return TRANSITIONS; }
{ALPHASTRING} {
               yylval.stringval = strndup(yytext, MAXTOKENLENGTH);
               return ALPHASTRING;
              }

"(" { return OPENBR; }
")" { return CLOSEBR; }
"," { return CMM; }

"/*" BEGIN(comment);
<comment>[^*\n]*        /* From flex man: eat anything that's not a '*' */
<comment>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
<comment>\n             /* ignore newlines */
<comment>"*"+"/"        BEGIN(INITIAL);

. { printf("ERR "); return ERROR; }

%%
