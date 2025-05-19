#include "global.h"
#include "symtab.h"
#include <stdarg.h>
// Definizioni costanti
char *filename = NULL;
int error_num = 0;
int main_flag = 0;
int current_scope_lvl = 1;             // indica il livello dello scope corrente
struct symlist *current_symtab = NULL; // tabella corrente
struct symlist *root_symtab = NULL;    // puntatore alla symbol table globale (scope più esterno)

/* Funzione di supporto alle funzioni per il print di errori, warning e note,
    prende come parametro una format string e un numero variabile di argomenti
    restituisce una nuova stringa dove sono inseriti i valori degli argomenti
*/
char *error_string_format(char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    int size = vsnprintf(NULL, 0, msg, args) + 1; // serve per restituire la dimensione di msg + args
    va_end(args);

    char *buffer = malloc(size);
    va_start(args, msg); // indica che il parametro msg precede la lista degli argomenti variabili
    vsprintf(buffer, msg, args);
    va_end(args);

    return buffer;
}