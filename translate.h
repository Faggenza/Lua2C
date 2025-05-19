#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "ast.h"

void translate(struct AstNode *root);
void translate_node(struct AstNode *n);
void translate_list(struct AstNode *l, const char* separator);
void translate_ast(struct AstNode *n);
#endif
