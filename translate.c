#include "translate.h"
#include "global.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "ast.h"
#include "pretty.h"
#include "semantic.h"
#include "symtab.h"


FILE *output_fp;          // File pointer per l'output C
int translate_depth = 0; // Per l'indentazione
int scope_lvl = 0; // Per la gestione degli scope

// Converte un LUA_TYPE nel corrispondente tipo stringa C
const char* lua_type_to_c_string(enum LUA_TYPE type) {
    switch (type) {
    case INT_T:     return "int";
    case FLOAT_T:   return "double";
    case STRING_T:  return "char*";
    case BOOLEAN_T: return "bool";
    case TRUE_T:    return "bool";
    case FALSE_T:   return "bool";
    case NIL_T:     return "/* nil_val_in_c */ void*"; // da vedere
    case FUNCTION_T: return "/* func_ptr_t */ void*";
    //case USERDATA_T: return "/* userdata_t */ void*";
    case NUMBER_T:  return "double";
    default:        return "/* unknown_type */ void*";
    }
}

// Stampa l'indentazione
void translate_tab() {
    for (int i = 0; i < translate_depth; i++) {
        fprintf(output_fp, "    "); // Usa 4 spazi per tab
    }
}

// Funzione per tradurre una lista di argomenti o espressioni
void translate_list(struct AstNode *l, const char* separator) {
    bool first = true;
    while(l){
        if (!first) {
            fprintf(output_fp, "%s", separator);
        }
        translate_node(l);
        first = false;
        l = l->next;
    }
}

// Funzione per tradurre il nodo
void translate_node(struct AstNode* n)
{
    if (!n) return;

    switch (n->nodetype)
    {
    case VAL_T:
        if (n->node.val->val_type == STRING_T) {
            fprintf(output_fp, "\"%s\"", n->node.val->string_val);
        } else if (n->node.val->val_type == TRUE_T) {
            fprintf(output_fp, "true");
        } else if (n->node.val->val_type == FALSE_T) {
            fprintf(output_fp, "false");
        } else if (n->node.val->val_type == NIL_T) {
            fprintf(output_fp, "NULL");
        } else {
            fprintf(output_fp, "%s", n->node.val->string_val);
        }
        break;
    case VAR_T:
        fprintf(output_fp, "%s", n->node.var->name);
        break;

    case EXPR_T:
        if (n->node.expr->expr_type == PAR_T) {
            fprintf(output_fp, "(");
            translate_node(n->node.expr->r);
            fprintf(output_fp, ")");
        } else if (n->node.expr->expr_type == NEG_T) {
            fprintf(output_fp, "-");
            translate_node(n->node.expr->r);
        } else if (n->node.expr->expr_type == NOT_T) {
            fprintf(output_fp, "!");
            translate_node(n->node.expr->r);
        } else if (n->node.expr->expr_type == AND_T)
        {
            fprintf(output_fp, "&&");
            translate_node(n->node.expr->l);
        }else {
            if(n -> node.expr -> l)
                translate_node(n -> node.expr -> l);
            fprintf(output_fp, "%s", convert_expr_type(n -> node.expr -> expr_type));
            translate_node(n -> node.expr -> r);
        }
        break;
    default:
        fprintf(output_fp, "/* unknown expression */");
        break;
    }
}


/* Funzione per tradurre l'Ast */
void translate_ast(struct AstNode *n){
    while(n){
        if(n -> nodetype != DECL_T){
            translate_tab(translate_depth);
        }
        if(n -> nodetype == FDEF_T || n -> nodetype == FOR_T || n -> nodetype == IF_T) {
            scope_lvl++;
            translate_depth++;
        }

        translate_node(n);

        if(n -> nodetype != DECL_T && n -> nodetype != FDEF_T && n -> nodetype != FOR_T && n -> nodetype != IF_T){
            fprintf(output_fp, ";\n");
        }

        n = n->next;
    }
}


void translate(struct AstNode *root)
{
    printf(">> Generated C translation!\n");
    output_fp = fopen("prova.c", "w");

    if(!output_fp) {
        fprintf(stderr, RED "error:" RESET " %s: ", filename);
        perror("");
        exit(1);
    }
    translate_ast(root);
}
