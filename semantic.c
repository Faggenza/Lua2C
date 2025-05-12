#include "semantic.h"
#include "global.h"
#include <string.h>
#include <stdlib.h>
#include "symtab.h"
#include <stdarg.h>
#include <stdio.h>

int div_by_zero; // flag che indica se ho una divisione per in eval_array_dim

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

/* Funzione per formattare messaggi di errore */
char *error_string_format(char *format, ...)
{
    static char buffer[1024];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    return buffer;
}

/* Evaluate expression type - in Lua this means inferring the runtime type */
struct complex_type eval_expr_type(struct AstNode *expr)
{
    struct complex_type result;

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

    case VAR_T:
        /* In Lua, variables can hold any type */
        result.type = expr->node.var ? DYNAMIC : NIL_T;
        result.kind = DYNAMIC;
        return result;

    case FCALL_T:
        /* Function calls can return any type in Lua */
        result.type = DYNAMIC;
        result.kind = DYNAMIC;
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

        case CONCAT_T:
            /* String concatenation */
            result.type = STRING_T;
            result.kind = DYNAMIC;
            return result;

        default:
            result.type = DYNAMIC;
            result.kind = DYNAMIC;
            return result;
        }

    case TABLE_T:
        result.type = TABLE_T;
        result.kind = DYNAMIC;
        return result;

    default:
        result.type = DYNAMIC;
        result.kind = DYNAMIC;
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
void check_fcall(char *name, struct AstNode *args)
{
    /* In Lua, function calls are much more flexible than in C */
    /* Check only if function exists - no parameter type checking */

    /* We could check for standard library functions or user-defined ones */
    /* For now, this is a simplified check */
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

        case CONCAT_T:
            /* String concatenation in Lua coerces to strings */
            break;

        default:
            /* No specific requirements for other operators */
            break;
        }
    }
}
