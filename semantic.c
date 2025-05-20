#include "semantic.h"
#include "global.h"
#include <string.h>
#include <stdlib.h>
#include "symtab.h"
#include <stdarg.h>
#include <stdio.h>

int div_by_zero; // flag che indica se ho una divisione per in eval_array_dim

enum LUA_TYPE eval_bool(char *t)
{
    switch (t[0])
    {
    case 't':
        return TRUE_T;
        break;
    case 'f':
        return FALSE_T;
        break;
    default:
        return ERROR_T;
        break;
    }
}

/* Controlla la correttezza della dimensione o indice di un array */
void check_array(struct AstNode *dim)
{
    /*  dim = dimensione o indice dell'array */
    if (check_array_dim(dim))
    {
        div_by_zero = 0;
        if (eval_array_dim(dim) < 0)
        {
            yyerror("cannot use negative array index or size");
        }
    }
}

/* Controlla se l'espressione è costante:
        ritorna 1: se l'espressione è costante intera
        ritorna 0: altrimenti
        errori: variabili,valori e funzioni con ritorno di tipo non intero
                operatori che restituiscono un valore booleano */
int check_array_dim(struct AstNode *expr)
{
    /*  expr = espressione che rappresenta la dimensione o indice di un array */
    switch (expr->nodetype)
    {
    case VAL_T:
        if (expr->node.val->val_type == FLOAT_T)
        {
            yyerror("invalid array index or size, non-integer len argument");
            return 0;
        }
        return 1;
    case VAR_T:;
        struct symbol *var = find_symtab(current_symtab, expr->node.var->name);
        if (var != NULL && var->type != INT_T)
            yyerror("invalid array index or size, non-integer len argument");
        return 0;
    case FCALL_T:;
        struct symbol *f = find_symtab(current_symtab, expr->node.fcall->name);
        if (f != NULL && f->type != INT_T)
            yyerror("invalid array index or size, non-integer len argument");
        return 0;
    case EXPR_T:;
        int l = 1;
        int r = 1;
        switch (expr->node.expr->expr_type)
        {
        case NOT_T:
        case AND_T:
        case OR_T:
        case G_T:
        case GE_T:
        case L_T:
        case LE_T:
        case EQ_T:
        case NE_T:
        case ASS_T:
            yyerror(error_string_format("invalid array index or size, cannot use operator " BOLD "%s" RESET, convert_expr_type(expr->node.expr->expr_type)));
            return 0;
        }

        if (expr->node.expr->l)
            l = check_array_dim(expr->node.expr->l);

        if (expr->node.expr->r)
            r = check_array_dim(expr->node.expr->r);

        return l && r;
    }
}

/* funzione richiamata solo se l'espressione è costante intera.
    Controlla che la dimensione/indice sia un numero intero >= 0 */
int eval_array_dim(struct AstNode *expr)
{
    /* Nota:    go restituisce un intero anche se il risultato è un numero decimale, l'importante è
                che si lavori con numeri int.
                esempio int a[3/2] non da errore anche se è float il risultato, lo approssima ad a[1] */
    /*  expr = espressione che rappresenta la dimensione o indice di un array */
    switch (expr->nodetype)
    {
        int l, r;
    case VAL_T:
        if (expr->node.val->val_type == INT_T)
            return atoi(expr->node.val->string_val);
    case EXPR_T:
        if (expr->node.expr->l)
            l = eval_array_dim(expr->node.expr->l);

        if (expr->node.expr->r)
            r = eval_array_dim(expr->node.expr->r);

        if (div_by_zero)
            return 0;

        switch (expr->node.expr->expr_type)
        {
        case ADD_T:
            return l + r;
        case SUB_T:
            return l - r;
        case MUL_T:
            return l * r;
        case DIV_T:
            if (r == 0)
            {
                div_by_zero = 1;
                return 0;
            }
            return l / r;
        case NEG_T:
            return -r;
        case PAR_T:
            return r;
        }
    }
}

/* Evaluate expression type - in Lua this means inferring the runtime type */
struct complex_type eval_expr_type(struct AstNode *expr)
{
    struct complex_type result;
    result.kind = DYNAMIC; // Default a dinamico per Lua

    if (!expr)
    {
        result.type = NIL_T;
        result.kind = CONSTANT;
        return result;
    }

    switch (expr->nodetype)
    {
    case VAL_T:
        result.type = expr->node.val->val_type;
        result.kind = CONSTANT;
        return result;

    case TABLE_NODE_T:
        // Per ora non supportato
        result.type = TABLE_T;
        result.kind = DYNAMIC;
        return result;
   case VAR_T:
            if (expr->node.var && expr->node.var->name) { // Controlla che il nome esista
                if (strcmp(expr->node.var->name, "io.read") == 0) {
                    result.type = FUNCTION_T; // "io.read" si riferisce a una funzione predefinita
                } else {
                    // Cerca la variabile nella symbol table corrente (o globale se necessario)
                    // È cruciale che 'current_symtab' (o un meccanismo di lookup simile)
                    // sia accessibile e punti allo scope corretto.
                    // Se eval_expr_type è chiamata ricorsivamente all'interno di funzioni,
                    // la gestione dello scope deve essere corretta.
                    // Usiamo 'root_symtab' come fallback per le globali se 'current_symtab'
                    // non è lo scope più esterno o non è disponibile.
                    // Se stai processando codice globale, current_symtab dovrebbe essere root_symtab.

                    struct symlist* scope_to_search = current_symtab;
                    if (!scope_to_search) { // Fallback se current_symtab è NULL per qualche motivo
                        scope_to_search = root_symtab;
                    }

                    struct symbol *sym = NULL;
                    if (scope_to_search) { // Solo cerca se abbiamo uno scope valido
                       sym = find_symtab(scope_to_search, expr->node.var->name);
                    }


                    if (sym) {
                        // Trovato il simbolo! Usa il suo tipo.
                        result.type = sym->type;
                        // Potresti anche voler impostare result.kind se la symbol table
                        // traccia se una variabile è una costante (meno comune in Lua).
                    } else {
                        // Variabile non trovata nella symbol table.
                        // In Lua, questo significherebbe che la variabile è 'nil'.
                        // Per la traduzione, questo è un punto critico.
                        // Potrebbe essere un errore (variabile non dichiarata se vuoi uno stile più statico)
                        // o potresti assumere 'nil' o un tipo dinamico.
                        // Per 'print', se una variabile non dichiarata viene stampata, Lua stampa 'nil'.
                        yywarning(error_string_format(
                            "Variable '%s' used in expression but not found in symbol table (or symbol table access issue). Assuming nil for now.",
                             expr->node.var->name
                        ));
                        result.type = NIL_T; // Comportamento più vicino a Lua per variabili non definite
                                             // In precedenza era USERDATA_T, che causava "userdata"
                    }
                }
            } else {
                // Nodo VAR_T senza nome, dovrebbe essere un errore del parser o AST
                yyerror("Encountered VAR_T node without a name during type evaluation.");
                result.type = ERROR_T;
            }
            return result;

    case FCALL_T:
        if (expr->node.fcall->func_expr->nodetype == VAR_T &&
            strcmp(expr->node.fcall->func_expr->node.var->name, "io.read") == 0)
        {
            // Tipo di ritorno specifico per io.read
            struct AstNode *fcall_args = expr->node.fcall->args;
            if (fcall_args == NULL)
            { // io.read() -> string (default "*l")
                result.type = STRING_T;
            }
            else
            {
                // Basato sul primo formato (semplificazione)
                if (fcall_args->nodetype == VAL_T)
                {
                    if (fcall_args->node.val->val_type == STRING_T)
                    {
                        const char *fmt = fcall_args->node.val->string_val;
                        if (strcmp(fmt, "*n") == 0)
                        {
                            result.type = NUMBER_T; // Potrebbe essere INT_T o FLOAT_T
                        }
                        else
                        { // "*l", "*a", "*L"
                            result.type = STRING_T;
                        }
                    }
                    else if (fcall_args->node.val->val_type == INT_T || fcall_args->node.val->val_type == FLOAT_T)
                    {
                        result.type = STRING_T; // io.read(N) legge N bytes come stringa
                    }
                    else
                    {
                        result.type = ERROR_T; // Formato non valido (già segnalato)
                    }
                }
                else
                {
                    result.type = ERROR_T; // Argomento non letterale, tipo sconosciuto a compile-time
                }
            }
        }
        else
        {
            // Per altre chiamate di funzione, il tipo di ritorno è generalmente dinamico
            // o deve essere cercato nella symbol table se la funzione è definita.
            result.type = USERDATA_T; // Placeholder per tipo di ritorno generico/dinamico
        }
        return result;

    case EXPR_T:
        switch (expr->node.expr->expr_type)
        {
        case ADD_T:
        case SUB_T:
        case MUL_T:
        case DIV_T:
            /* May be number or string (for ADD_T) in Lua */
            result.type = NUMBER_T;
            result.kind = DYNAMIC;
            return result;

        case AND_T:
        case OR_T:
        case NOT_T:
        case G_T:
        case GE_T:
        case L_T:
        case LE_T:
        case EQ_T:
        case NE_T:
            /* All comparison/logical ops return boolean */
            result.type = BOOLEAN_T;
            result.kind = DYNAMIC;
            return result;

        case NEG_T:
            /* Unary minus keeps type */
            return eval_expr_type(expr->node.expr->r);

        case ASS_T:
            /* Assignment returns RHS value in Lua */
            return eval_expr_type(expr->node.expr->r);

        case PAR_T:
            /* Parentheses preserve type */
            return eval_expr_type(expr->node.expr->r);

        default:
            result.type = DYNAMIC;
            result.kind = DYNAMIC;
            return result;
        }
    default:
        // Tipo di nodo AST non gestito esplicitamente qui
        result.type = USERDATA_T; // O ERROR_T
        return result;
    }
}

/* Check conditional expressions */
void check_cond(enum LUA_TYPE type)
{
    /* In Lua, any value except nil and false is considered true */
    /* No actual checking needed - all types can be conditions */
}

/* Check that division doesn't involve zero */
void check_division(struct AstNode *expr)
{
    struct complex_type t = eval_expr_type(expr);

    /* Can only check if it's a constant */
    if (t.kind == CONSTANT && t.type == NUMBER_T)
    {
        if (atof(expr->node.val->string_val) == 0)
        {
            yyerror("division by zero");
        }
    }
}

/* Check if an expression is used as a statement */
struct AstNode *check_expr_statement(struct AstNode *expr)
{
    /* In Lua, expressions can be statements without assignment */
    /* Only function calls, assignments, and variable declarations have effects */
    if (expr->nodetype != FCALL_T && expr->nodetype != EXPR_T)
    {
        yywarning("expression without effect");
    }
    return expr;
}

/* Check function call */
// void check_fcall(char *name, struct AstNode *args)
//  {
/* In Lua, function calls are much more flexible than in C */
/* Check only if function exists - no parameter type checking */

/* We could check for standard library functions or user-defined ones */
/* For now, this is a simplified check */
// }

void check_fcall(struct AstNode *func_expr, struct AstNode *args)
{
    if (func_expr->nodetype == VAR_T &&
        strcmp(func_expr->node.var->name, "io.read") == 0)
    {
        // È una chiamata a io.read
        struct AstNode *current_arg = args;
        int arg_count = 0;
        if (current_arg != NULL)
        {                  // Se ci sono argomenti
            arg_count = 1; // Iniziamo a contare
            // Per semplicità, consideriamo solo il primo argomento per i controlli dettagliati.
            // Lua permette più argomenti, che portano a più valori di ritorno.

            if (current_arg->nodetype == VAL_T)
            {
                if (current_arg->node.val->val_type == STRING_T)
                {
                    const char *fmt = current_arg->node.val->string_val;
                    if (strcmp(fmt, "*n") == 0 || strcmp(fmt, "*l") == 0 ||
                        strcmp(fmt, "*a") == 0 || strcmp(fmt, "*L") == 0)
                    {
                        // Formato stringa valido
                    }
                    else
                    {
                        // In Lua, io.read("qualsiasi_stringa_non_formato") è un errore runtime.
                        // Qui potremmo essere più restrittivi.
                        yyerror("io.read: argument must be a format string or a number");
                    }
                }
                else if (current_arg->node.val->val_type == INT_T || current_arg->node.val->val_type == FLOAT_T)
                {
                    // Formato numerico (numero di byte da leggere)
                    if (current_arg->node.val->val_type == FLOAT_T)
                    {
                        yywarning("io.read: numeric format is float, will be treated as integer");
                    }
                    // Potresti aggiungere un controllo se è negativo: atof(current_arg->node.val->string_val) < 0
                }
                else
                {
                    yyerror("io.read: argument must be a format string or a number");
                }
            }
            else
            {
                // L'argomento di io.read (se presente) deve essere una costante stringa o numerica
                // per questa analisi statica.
                yyerror("io.read: argument must be a literal format string or number");
            }

            if (current_arg->next != NULL)
            {
                yywarning("io.read: multiple arguments provided. Translation to C might only support the first or be complex.");
                // Qui potresti iterare su current_arg->next per controllare anche gli altri,
                // ma la gestione di ritorni multipli è il vero problema per la traduzione.
            }
        }
        // Se arg_count == 0 (cioè args è NULL), è io.read() che è valido (default "*l").
    }
    else if (func_expr->nodetype == VAR_T)
    {
        // Chiamata a una funzione normale (es. f())
        // Qui puoi cercare func_expr->node.var->name nella symbol table,
        // controllare i parametri, etc. come facevi prima con check_fcall(char* name, ...)
        // struct symbol* func_sym = find_symtab(current_symtab, func_expr->node.var->name);
        // if (!func_sym) { yyerror("call to undefined function '%s'", func_expr->node.var->name); }
        // ... ecc.
    }
    else
    {
        // Chiamata a qualcosa che non è un ID semplice (es. (get_func())() )
        // Questo è più complesso e potrebbe non essere supportato ora.
        yywarning("calling a complex expression as a function is not fully checked yet");
    }
}

/* Check returned values */
void check_return(struct AstNode *expr)
{
    /* In Lua, return values are dynamically typed */
    /* No checking required for type correctness */
}

/* Check variable references */
void check_var_reference(struct AstNode *var)
{
    /* In Lua, accessing an undefined variable returns nil */
    /* This is not an error, but we might want to warn about it */
}

/* Check table access */
void check_table_access(struct AstNode *table, struct AstNode *key)
{
    /* In Lua, accessing a non-existent table key returns nil */
    /* No error, but we could warn about potential issues */
}

/* Check variable assignment */
void check_var_assignment(struct AstNode *var, struct AstNode *expr)
{
    /* In Lua, variables can be assigned any type */
    /* No type checking required */
}

/* Check format strings (for print) */
void check_format_string(struct AstNode *format_string, struct AstNode *args, enum FUNC_TYPE f_type)
{
    /* In Lua, string formatting works differently - could implement a simple check */
    /* For compatibility with the existing code, we'll leave this function but simplify it */
}

/* Controllo del valore di ritorno delle funzioni */
void check_func_return(enum LUA_TYPE type, struct AstNode *stmt_list)
{
    /* In Lua, le funzioni possono restituire qualsiasi tipo o nessun valore */
    /* Non è necessario il controllo del tipo di ritorno */
}

/* Evaluate constant expressions */
int eval_constant_expr(struct AstNode *expr)
{
    if (!expr)
        return 0;

    switch (expr->nodetype)
    {
    case VAL_T:
        if (expr->node.val->val_type == NUMBER_T)
        {
            return atof(expr->node.val->string_val);
        }
        return 0;

    case EXPR_T:
        switch (expr->node.expr->expr_type)
        {
        case ADD_T:
            return eval_constant_expr(expr->node.expr->l) + eval_constant_expr(expr->node.expr->r);
        case SUB_T:
            return eval_constant_expr(expr->node.expr->l) - eval_constant_expr(expr->node.expr->r);
        case MUL_T:
            return eval_constant_expr(expr->node.expr->l) * eval_constant_expr(expr->node.expr->r);
        case DIV_T:
        {
            int divisor = eval_constant_expr(expr->node.expr->r);
            if (divisor == 0)
            {
                yyerror("division by zero in constant expression");
                return 0;
            }
            return eval_constant_expr(expr->node.expr->l) / divisor;
        }
        case NEG_T:
            return -eval_constant_expr(expr->node.expr->r);

        default:
            return 0;
        }

    default:
        return 0;
    }
}

/* Check binary operations for valid operand types */
void check_binary_op(enum EXPRESSION_TYPE op, struct AstNode *left, struct AstNode *right)
{
    /* In Lua, most operators are flexible about types */
    /* The only strict requirements are for arithmetic operations (need numbers) */

    struct complex_type l_type = eval_expr_type(left);
    struct complex_type r_type = eval_expr_type(right);

    /* Only perform checks for constant expressions */
    if (l_type.kind == CONSTANT && r_type.kind == CONSTANT)
    {
        switch (op)
        {
        case ADD_T:
        case SUB_T:
        case MUL_T:
        case DIV_T:
            /* Require numbers for arithmetic */
            if (l_type.type != NUMBER_T || r_type.type != NUMBER_T)
            {
                yywarning("arithmetic operation on non-numeric operands");
            }
            break;


        default:
            /* No specific requirements for other operators */
            break;
        }
    }
}
