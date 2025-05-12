%option yylineno

%x COMMENT
%x DQUOTE
%x SQUOTE

%{
#include "parser.tab.h"
#include <string.h>
#include <stdarg.h>
#include "global.h"

char *line;
void copy_line();
char string_buf[230000]; // Max Lua string length

%}

%%

^.*                         { copy_line(); }

    /* commenti */

"--[["                      { BEGIN COMMENT; }
<COMMENT>[^\]]*             { /* Match non-closing bracket chars */ }
<COMMENT>\][^\]]*           { /* Match ] followed by non-] chars */ }
<COMMENT>"]]"               { BEGIN INITIAL; }
<COMMENT><<EOF>>            { yyerror("unterminated comment"); BEGIN INITIAL; }



("--".*)                    { }

    /* stringhe */
\"\"                     { yylval.s = strdup(""); return STRING; }
\'\'                     { yylval.s = strdup(""); return STRING; }

\"                        { BEGIN DQUOTE; string_buf[0] = '\0'; }
<DQUOTE>([^"\\\n]|\\.)+   { strcat(string_buf, yytext); }
<DQUOTE>\"                { BEGIN INITIAL; yylval.s = strdup(string_buf); return STRING; }
<DQUOTE>\n |
<DQUOTE><<EOF>>             { yyerror("missing terminating \" character"); BEGIN INITIAL; }

\'                        { BEGIN SQUOTE; string_buf[0] = '\0'; }
<SQUOTE>([^'\\\n]|\\.)+   { strcat(string_buf, yytext); }
<SQUOTE>\'                { BEGIN INITIAL; yylval.s = strdup(string_buf); return STRING; }
<SQUOTE>\n |
<SQUOTE><<EOF>>             { yyerror("missing terminating ' character"); BEGIN INITIAL; }

    /* costanti numeriche */

[0]+ |
[1-9][0-9]*                 { yylval.s = strdup(yytext); return INT_NUM; }

[0]+[0-9]+                  { yyerror("octal literal not allowed"); }

([0-9]+)?(\.[0-9]+) |
([0-9]+)\. |
([0-9]+)(e|E)(\+|-)?[0-9]+ |
([0-9]+)?(\.[0-9]+)(e|E)(\+|-)?[0-9]+ |
(([0-9]+)\.)(e|E)(\+|-)?[0-9]+   { yylval.s = strdup(yytext); return FLOAT_NUM; }

    /* keyword */

"and"           { return AND; }
"do"            { return DO; }
"else"          { return ELSE; }
"end"           { return END; }
"false"         { yylval.s = strdup(yytext); return BOOL; }
"true"          { yylval.s = strdup(yytext); return BOOL; }
"for"           { return FOR; }
"function"      { return FUNCTION; }
"if"            { return IF; }
"nil"           { return NIL; }
"not"           { return NOT; }
"or"            { return OR; }
"return"        { return RETURN; }
"then"          { return THEN; }


    /* identificatori */

[_a-zA-Z][_a-zA-Z0-9]*      { yylval.s = strdup(yytext); return ID; }

    /* operatori aritmentici */

"+" |
"-" |
"*" |
"/" |
"="                         { return yytext[0]; }




    /* operatori di confronto */

">" |
"<"                         { return yytext[0]; }                    
">="                        { return GE; }
"<="                        { return LE; }
"=="                        { return EQ; }
"~="                        { return NE; }

    /* altri simboli */

"{"                         { return yytext[0]; }
"}"                         { return yytext[0]; }
"(" |
")" |
"[" |
"]" |
";" |
"," |
"&"                         { return yytext[0]; }

    /* spazi e tabulazioni */

[ \t\v\f\n]                 { }

    /* fallback per gli errori */

.                           { yyerror(error_string_format("invalid character" BOLD " %s" RESET, yytext)); }

%%

/* Copia le righe dell'input */
void copy_line() {
    if(line) {
        free(line);
    }

    line = malloc(sizeof(char) * (yyleng + 1)); 
    strcpy(line, yytext); 
    // restituisce la linea al buffer di input per matcharlo con le regole successive
    yyless(0); 
}

/* Printa gli errori sullo standard error e mantiene un contatore degli errori */
void yyerror(const char *s) {
    fprintf(stderr, "%s:%d " RED "error:" RESET " %s\n", filename, yylineno, s);
    fprintf(stderr, "%s\n", line);
    error_num++;
}

/* Printa i warning sullo standard error */
void yywarning(char *s) {
    /* per stamparlo su stderr */
    fprintf(stderr, "%s:%d " YELLOW "warning:" RESET " %s\n", filename, yylineno, s);
    fprintf(stderr, "%s\n", line);
}

/* Printa delle note sullo stantard error (tipicamente associate ad errori o warning) */
void yynote(char *s, int lineno, char *line){
    fprintf(stderr, "%s:%d " BLUE "note:" RESET " %s\n", filename, lineno, s);
    fprintf(stderr, " %s\n", line);
}

