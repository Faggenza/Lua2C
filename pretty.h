#include <stdio.h>
#include "ast.h"
/* Funzioni per il print dell'Ast */
void print_ast(struct AstNode *n);
void print_node(struct AstNode *n);
void print_list(struct AstNode *l);
void print_decl(struct AstNode *l, char *type);
void print_tab(int depth);

char *convert_expr_type(enum EXPRESSION_TYPE expr_type);
char *convert_var_type(enum LUA_TYPE type);
char *convert_node_type(enum NODE_TYPE nodetype);
char *convert_func_name(char *name);
