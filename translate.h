#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "ast.h"
#include "symtab.h"

// Struttura per tenere traccia degli scope durante la traduzione
struct scope_tracker
{
    int level;
    struct symlist *current_scope;
};

void translate(struct AstNode *root);
void translate_node(struct AstNode *n, struct symlist *current_scope);
void translate_list(struct AstNode *l, const char *separator);
void translate_ast(struct AstNode *n);
void translate_params(struct AstNode *params);
const char *lua_type_to_c_string(enum LUA_TYPE type);
#endif
