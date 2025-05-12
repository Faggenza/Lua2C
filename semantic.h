#include "ast.h"
#include "pretty.h"
/* Types of built-in functions */
enum FUNC_TYPE
{
    PRINT_T,
    READ_T
};

/* Complex type for expression evaluation */
struct complex_type
{
    enum LUA_TYPE type;
    enum
    {
        DYNAMIC, // Value determined at runtime
        CONSTANT // Value known at compile time
    } kind;
};

/* Array checking functions */
void check_array(struct AstNode *dim);
int check_array_dim(struct AstNode *expr);
int eval_array_dim(struct AstNode *expr);

/* Function checks */
void check_fcall(char *name, struct AstNode *args);
void check_func_return(enum LUA_TYPE type, struct AstNode *stmt_list);
void check_return(struct AstNode *expr);
void check_main_chunk(struct AstNode *chunk);

/* Table-related checks */
void check_table_access(struct AstNode *table, struct AstNode *key);
int eval_constant_expr(struct AstNode *expr);

/* Variable checks */
void check_var_reference(struct AstNode *var);
void check_var_assignment(struct AstNode *var, struct AstNode *expr);

/* Format string functions */
void check_format_string(struct AstNode *format_string, struct AstNode *args, enum FUNC_TYPE f_type);

/* Expression checks */
struct complex_type eval_expr_type(struct AstNode *expr);
enum LUA_TYPE eval_bool(char *t);
void check_cond(enum LUA_TYPE type);
void check_binary_op(enum EXPRESSION_TYPE op, struct AstNode *left, struct AstNode *right);
void check_division(struct AstNode *expr);
struct AstNode *check_expr_statement(struct AstNode *expr);
