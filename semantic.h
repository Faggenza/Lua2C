#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symtab.h"

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

void check_fcall(struct AstNode* func_expr, struct AstNode* args);
int eval_constant_expr(struct AstNode* expr);
struct complex_type eval_expr_type(struct AstNode* expr, struct symlist* current_scope);
enum LUA_TYPE infer_func_return_type(struct AstNode* code, struct symlist* func_scope);
enum LUA_TYPE eval_bool(char* t);
void check_division(struct AstNode* expr, struct symlist* current_scope);

#endif
