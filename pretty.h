#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <stdio.h>

/* Funzioni per il print dell'Ast */
void print_ast(struct AstNode *n);
void print_node(struct AstNode *n);
void print_list(struct AstNode *l);
void print_decl(struct AstNode *l, char * type);

