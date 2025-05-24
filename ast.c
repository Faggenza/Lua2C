#include "ast.h"
#include "semantic.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Inferisce il tipo da un valore stringa */
enum LUA_TYPE infer_type(char *value)
{
    if (!value)
        return NIL_T;
    if (strcmp(value, "nil") == 0)
        return NIL_T;
    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0)
        return BOOLEAN_T;

    // Controlla se è un numero
    char *endptr;
    double num = strtod(value, &endptr);
    if (*endptr == '\0')
    {
        if (strchr(value, '.'))
            return FLOAT_T;
        else
            return INT_T;
    }

    // Altrimenti è una stringa
    return STRING_T;
}

/* Crea un nodo valore - ora accetta il tipo esplicitamente */
struct AstNode *new_value(enum NODE_TYPE nodetype, enum LUA_TYPE val_type, char *string_val)
{
    struct value *val = malloc(sizeof(struct value));
    struct AstNode *node = malloc(sizeof(struct AstNode));

    val->val_type = val_type != 0 ? val_type : infer_type(string_val);
    val->string_val = string_val;

    node->nodetype = nodetype;
    node->node.val = val;
    node->next = NULL;

    return node;
}

/* Crea un nodo variabile */
struct AstNode *new_variable(enum NODE_TYPE nodetype, char *name, struct AstNode *table_key)
{
    struct variable *var = malloc(sizeof(struct variable));
    struct AstNode *node = malloc(sizeof(struct AstNode));

    var->name = name;
    var->table_key = table_key;

    node->nodetype = nodetype;
    node->node.var = var;
    node->next = NULL;

    return node;
}

/* Modifica il valore del campo reference di un nodo di tipo VAR_T */
void by_reference(struct AstNode *node)
{
    node->node.var->by_reference = 1;
}

/* Crea un nodo dichiarazione - non include tipi espliciti in Lua */
struct AstNode *new_declaration(enum NODE_TYPE nodetype, struct AstNode *var, struct AstNode *expr)
{
    struct declaration *decl = malloc(sizeof(struct declaration));
    struct AstNode *node = malloc(sizeof(struct AstNode));

    decl->var = var;
    decl->expr = expr;

    node->nodetype = nodetype;
    node->node.decl = decl;
    node->next = NULL;

    return node;
}

/* Crea un nodo espressione */
struct AstNode *new_expression(enum NODE_TYPE nodetype, enum EXPRESSION_TYPE expr_type, struct AstNode *l, struct AstNode *r)
{
    struct expression *expr = malloc(sizeof(struct expression));
    struct AstNode *node = malloc(sizeof(struct AstNode));

    expr->expr_type = expr_type;
    expr->l = l;
    expr->r = r;

    node->nodetype = nodetype;
    node->node.expr = expr;
    node->next = NULL;

    return node;
}

/* Crea un nuovo nodo return */
struct AstNode *new_return(enum NODE_TYPE nodetype, struct AstNode *expr)
{
    struct returnNode *rnode = malloc(sizeof(struct returnNode));
    struct AstNode *node = malloc(sizeof(struct AstNode));

    rnode->expr = expr;

    node->nodetype = nodetype;
    node->node.ret = rnode;
    node->next = NULL;

    return node;
}

/* Crea un nodo chiamata a funzione */
struct AstNode *new_func_call(enum NODE_TYPE nodetype, struct AstNode *func_expr, struct AstNode *args)
{
    struct funcCall *fcall = malloc(sizeof(struct funcCall));
    struct AstNode *node = malloc(sizeof(struct AstNode));

    fcall->func_expr = func_expr;
    fcall->args = args;
    fcall->return_type = NIL_T; // Default return type, will be updated by check_fcall if function is found

    node->nodetype = nodetype;
    node->node.fcall = fcall;
    node->next = NULL;

    return node;
}

/* Crea un nodo definizione di funzione */
struct AstNode *new_func_def(enum NODE_TYPE nodetype, char *name, struct AstNode *params, struct AstNode *code, enum LUA_TYPE ret_type)
{
    struct funcDef *fdef = malloc(sizeof(struct funcDef));
    struct AstNode *node = malloc(sizeof(struct AstNode));

    fdef->name = name;
    fdef->params = params;
    fdef->code = code;
    fdef->ret_type = ret_type;

    node->nodetype = nodetype;
    node->node.fdef = fdef;
    node->next = NULL;

    return node;
}

/* Crea un nodo for */
struct AstNode *new_for(enum NODE_TYPE nodetype, char *varname, struct AstNode *start, struct AstNode *end, struct AstNode *step, struct AstNode *stmt)
{
    struct forNode *forn = malloc(sizeof(struct forNode));
    struct AstNode *node = malloc(sizeof(struct AstNode));

    forn->varname = varname;
    forn->start = start;
    forn->end = end;
    forn->step = step;
    forn->stmt = stmt;

    node->nodetype = nodetype;
    node->node.forn = forn;
    node->next = NULL;

    return node;
}

/* Crea un nodo while */
/*struct AstNode *new_while(enum NODE_TYPE nodetype, struct AstNode *cond, struct AstNode *body)
{
    struct whileNode *whilen = malloc(sizeof(struct whileNode));
    struct AstNode *node = malloc(sizeof(struct AstNode));

    whilen->cond = cond;
    whilen->body = body;

    node->nodetype = nodetype;
    node->node.whilen = whilen;
    node->next = NULL;

    return node;
}*/

/* Crea un nodo if */
struct AstNode *new_if(enum NODE_TYPE nodetype, struct AstNode *cond, struct AstNode *body, struct AstNode *else_body)
{
    struct ifNode *ifn = malloc(sizeof(struct ifNode));
    struct AstNode *node = malloc(sizeof(struct AstNode));

    ifn->cond = cond;
    ifn->body = body;
    ifn->else_body = else_body;

    node->nodetype = nodetype;
    node->node.ifn = ifn;
    node->next = NULL;

    return node;
}

/* Crea un nodo tabella */

struct AstNode *new_table(enum NODE_TYPE nodetype, struct AstNode *fields)
{
    struct table *t = malloc(sizeof(struct table));
    struct AstNode *node = malloc(sizeof(struct AstNode));

    t->fields = fields;

    node->nodetype = nodetype;
    node->node.table = t;
    node->next = NULL;

    return node;
}

/* Crea un nodo campo di tabella */
struct AstNode *new_table_field(enum NODE_TYPE nodetype, struct AstNode *key, struct AstNode *value)
{
    struct tableField *field = malloc(sizeof(struct tableField));
    struct AstNode *node = malloc(sizeof(struct AstNode));

    field->key = key;
    field->value = value;

    node->nodetype = nodetype;
    node->node.tfield = field;
    node->next = NULL;

    return node;
}

/* Crea un nodo errore */
struct AstNode *new_error(enum NODE_TYPE nodetype)
{
    struct AstNode *node = malloc(sizeof(struct AstNode));

    node->nodetype = nodetype;
    node->next = NULL;
    return node;
}

/* Funzione usata per creare una lista di nodi AST partendo dall'ultimo */
struct AstNode *link_AstNode(struct AstNode *node, struct AstNode *next)
{
    node->next = next;
    return node;
}

/* Funzione usata per creare una lista di nodi AST partendo dal primo */
struct AstNode *append_AstNode(struct AstNode *node, struct AstNode *next)
{
    node->next = next;
    return next;
}
