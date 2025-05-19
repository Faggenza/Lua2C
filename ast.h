#ifndef AST_H
#define AST_H

// Tipo di dato in Lua - dinamicamente determinato
enum LUA_TYPE
{
    NIL_T,
    BOOLEAN_T,
    TRUE_T,
    FALSE_T,
    NUMBER_T,
    INT_T,
    FLOAT_T,
    STRING_T,
    FUNCTION_T,
    TABLE_T,
    USERDATA_T,
    ERROR_T
};

// Tipo di nodo
enum NODE_TYPE
{
    EXPR_T,
    VAL_T,
    VAR_T,
    TABLE_FIELD_T,
    TABLE_NODE_T,
    DECL_T,
    RETURN_T,
    FCALL_T,
    FDEF_T,
    IF_T,
    FOR_T,
    WHILE_T,
    END_T,
    ERROR_NODE_T
};

// Tipo di espressione
enum EXPRESSION_TYPE
{
    ASS_T, // =
    ADD_T,
    SUB_T,
    DIV_T,
    MUL_T,
    NOT_T,
    AND_T,
    OR_T,
    G_T,
    GE_T,
    L_T,
    LE_T,
    EQ_T,
    NE_T,
    NEG_T,
    PAR_T,
    CONCAT_T // .. operatore concatenazione stringhe di Lua
};

// Struttura del nodo espressione
struct expression
{
    enum EXPRESSION_TYPE expr_type;
    struct AstNode *l;
    struct AstNode *r;
};

// Struttura del nodo if
struct ifNode
{
    struct AstNode *cond;
    struct AstNode *body;
    struct AstNode *else_body;
};

// Struttura del nodo for
struct forNode
{
    char *varname;         // variabile del for (i)
    struct AstNode *start; // valore iniziale (1)
    struct AstNode *end;   // valore finale (10)
    struct AstNode *step;  // step (opzionale, può essere NULL)
    struct AstNode *stmt;  // corpo del ciclo
};

// Struttura del nodo while
struct whileNode
{
    struct AstNode *cond;
    struct AstNode *body;
};

// Struttura del nodo valore
struct value
{
    enum LUA_TYPE val_type; // Determinato dinamicamente
    char *string_val;
};

// Struttura del nodo variabile
struct variable
{
    char *name;
    struct AstNode *table_key; // Per accesso alla tabella: t["key"] o t.key
    int by_reference;          // 1 se per riferimento, 0 se per valore
};

// Struttura per una tabella Lua
struct table
{
    struct AstNode *fields; // Lista di campi della tabella
};

// Struttura per un campo di tabella Lua
struct tableField
{
    struct AstNode *key; // Può essere nil per indici numerici automatici
    struct AstNode *value;
};

// Struttura del nodo dichiarazione locale
struct declaration
{
    struct AstNode *var;  // Non conserva tipo - in Lua è dinamico
    struct AstNode *expr; // Espressione di inizializzazione (opzionale)
};

// Struttura del nodo return
struct returnNode
{
    struct AstNode *expr; // Può essere una lista di espressioni in Lua
};

// Struttura del nodo chiamata a funzione
struct funcCall
{
    char *name;                // Può essere nil per funzioni anonime
    struct AstNode *func_expr; // Può essere un nome o un'espressione che valuta a funzione
    struct AstNode *args;
};

// Struttura del nodo definizione di funzione
struct funcDef
{
    char *name; // Può essere nil per funzioni anonime
    struct AstNode *params;
    struct AstNode *code;
};

// Struttura del nodo generico Ast
struct AstNode
{
    enum NODE_TYPE nodetype;

    union node
    {
        struct variable *var;
        struct value *val;
        struct expression *expr;
        struct ifNode *ifn;
        struct forNode *forn;
        // struct whileNode *whilen;
        struct declaration *decl;
        struct returnNode *ret;
        struct funcCall *fcall;
        struct funcDef *fdef;
        struct table *table;
        struct tableField *tfield;
    } node;

    struct AstNode *next;
};

/* Funzioni per creare i nodi */
struct AstNode *new_value(enum NODE_TYPE nodetype, enum LUA_TYPE val_type, char *string_val);
struct AstNode *new_variable(enum NODE_TYPE nodetype, char *name, struct AstNode *table_key);
struct AstNode *new_expression(enum NODE_TYPE nodetype, enum EXPRESSION_TYPE expr_type, struct AstNode *l, struct AstNode *r);
struct AstNode *new_declaration(enum NODE_TYPE nodetype, struct AstNode *var, struct AstNode *expr);
struct AstNode *new_return(enum NODE_TYPE nodetype, struct AstNode *expr);
struct AstNode *new_func_call(enum NODE_TYPE nodetype, struct AstNode *func_expr, struct AstNode *args);
struct AstNode *new_func_def(enum NODE_TYPE nodetype, char *name, struct AstNode *params, struct AstNode *code);
struct AstNode *new_while(enum NODE_TYPE nodetype, struct AstNode *cond, struct AstNode *body);
struct AstNode *new_for(enum NODE_TYPE nodetype, char *varname, struct AstNode *start, struct AstNode *end, struct AstNode *step, struct AstNode *stmt);
struct AstNode *new_if(enum NODE_TYPE nodetype, struct AstNode *cond, struct AstNode *body, struct AstNode *else_body);
struct AstNode *new_table(enum NODE_TYPE nodetype, struct AstNode *fields);
struct AstNode *new_table_field(enum NODE_TYPE nodetype, struct AstNode *key, struct AstNode *value);
struct AstNode *new_error(enum NODE_TYPE nodetype);

/* Funzioni per linkare due nodi */
struct AstNode *link_AstNode(struct AstNode *node, struct AstNode *next);
struct AstNode *append_AstNode(struct AstNode *node, struct AstNode *next);

/* Funzioni per inferire i tipi */
enum LUA_TYPE infer_type(char *value);

#endif
