%option yylineno

%x COMMENT
%x DQUOTE

%{
#include "token.h"
#include <string.h>
#include <stdarg.h>

char *line;
void copy_line();

%}

%%

^.*                         { copy_line(); }

    /* commenti */

"--[["                      { BEGIN COMMENT; }
<COMMENT>[^\]]*             { } /* Match any characters except ] */
<COMMENT>"]"[^\]]*          { } /* Match ] followed by non-] characters */
<COMMENT>"]]"               { BEGIN INITIAL; }
<COMMENT><<EOF>>            { yyerror("unterminated comment"); BEGIN INITIAL; }



("--".*)                    { }

    /* stringhe */

\"\"                     { yylval.s = ""; return STRING; }

\"                        { BEGIN DQUOTE; }
<DQUOTE>([^"'\\\n]|\\.)*     { yylval.s = strdup(yytext); return STRING; }
<DQUOTE>\"                { BEGIN INITIAL; }
<DQUOTE>\n |
<DQUOTE><<EOF>>             { yyerror("missing terminating \" character"); BEGIN INITIAL; }

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
"break"         { return BREAK; }
"do"            { return DO; }
"else"          { return ELSE; }
"elseif"        { return ELSEIF; }
"end"           { return END; }
"false"         { return FALSE; }
"for"           { return FOR; }
"function"      { return FUNCTION; }
"goto"          { return GOTO; }
"if"            { return IF; }
"in"            { return IN; }
"local"         { return LOCAL; }
"nil"           { return NIL; }
"not"           { return NOT; }
"or"            { return OR; }
"repeat"        { return REPEAT; }
"return"        { return RETURN; }
"then"          { return THEN; }
"true"          { return TRUE; }
"until"         { return UNTIL; }
"while"         { return WHILE; }

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

/* Funzione variadica di supporto alle funzioni per il print di errori, warning e note,
    prende come parametro una format string e un numero variabile di argomenti
    restituisce una nuova stringa dove sono inseriti i valori degli argomenti  
*/
char* error_string_format(char *msg, ...) {
    va_list args;

    va_start(args, msg);
    int size = vsnprintf(NULL, 0, msg, args) + 1; // serve per restituire la dimensione di msg + args
    va_end(args);

    char *buffer = malloc(size);
    va_start(args, msg); //indica che il parametro msg precede la lista degli argomenti variabili
    vsprintf(buffer, msg, args);
    va_end(args);
    
    return buffer;
}
