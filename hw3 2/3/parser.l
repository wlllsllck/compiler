%{
#include "parser.h"
%}

%option noyywrap

%%

[ \t\n]+       { /* ignore */ }
"procedure"    return(PROCEDURE);
"{"            return(BLOCK);
"}"            return(ENDBLOCK);
[_a-zA-Z0-9]+  {
      				yylval.sval = malloc(strlen(yytext));
      				strncpy(yylval.sval, yytext, strlen(yytext));
      				return(IDENTIFIER);
				}
%%