#include "tree.h"
#include "global.h"
#include <stdlib.h>
#include "translate.h"

/* Funzione per printare i nodi Ast */
void print_node(struct AstNode *n){
    switch(n->nodetype){
        case VAL_T:
            if(n -> node.val -> val_type == STRING_T) {
                printf("\"");
                printf("%s", n -> node.val -> string_val);
                printf("\"");
            }else{
                printf("%s", n -> node.val -> string_val);
            }
            break;
        case VAR_T:
            if(n -> node.var -> by_reference) {
                printf("&");
            }
            printf("%s", n -> node.var-> name);
            if(n -> node.var -> array_dim) {
                printf("[");
                print_node(n -> node.var -> array_dim);
                printf("]");
            }
            break;
        case DECL_T:
            printf("%s ", convert_var_type(n -> node.decl -> type));
            print_list(n -> node.decl -> var);
            break;
        case EXPR_T:
            if(n -> node.expr -> expr_type == PAR_T) {
                printf("(");
                print_node(n -> node.expr -> r);
                printf(")");
            } else {
                if(n -> node.expr -> l)
                    print_node(n -> node.expr -> l);
                printf("%s", convert_expr_type(n -> node.expr -> expr_type));
                print_node(n -> node.expr -> r);
            }
            break;
        case RETURN_T:
            printf("return ");
            if(n -> node.ret -> expr) {
                print_node(n -> node.ret -> expr);
            }
            break;
        case FCALL_T:
            printf("%s(", n -> node.fcall -> name);
            if(n -> node.fcall -> args) {
                print_list(n-> node.fcall -> args);
            }
            printf(")");
            break;
        case FDEF_T:
            printf("%s ", convert_var_type(n -> node.fdef -> ret_type));
            printf("%s(", n -> node.fdef -> name );
            if (n -> node.fdef -> params){
                print_list(n -> node.fdef -> params);
            }
            printf(") {\n");
            print_ast( n -> node.fdef -> code);
            depth--;
            print_tab(depth);
            printf("}\n\n");
            break;
        case FOR_T:
            printf("for(");
            if(n -> node.forn -> init)
                print_node(n -> node.forn -> init);
            printf("; ");
            if(n -> node.forn -> cond)
                print_node(n -> node.forn -> cond);
            printf("; ");
            if(n -> node.forn -> update)
                print_node(n -> node.forn -> update);
            printf(") {\n");
            print_ast(n -> node.forn -> stmt);
            depth--;
            print_tab(depth);
            printf("}\n");
            break;
        case IF_T:
            printf("if(");
            print_node(n -> node.ifn -> cond);
            printf(") {\n");
            print_ast(n -> node.ifn -> body);
            depth--;
            print_tab(depth);
            printf("}");
            if(n -> node.ifn -> else_body) {
                printf("else{\n");
                depth++;
                print_ast(n -> node.ifn -> else_body);
                depth--;
                print_tab(depth);
                printf("}\n");
            }else{
                printf("\n");
            }
            break;
        case ERROR_NODE_T:
            printf("error node");
    }
}

/* Funzione per printare l'Ast */
void print_ast(struct AstNode *n) {
    while(n){
        print_tab(depth);

        if(n -> nodetype == FDEF_T || n -> nodetype == FOR_T || n -> nodetype == IF_T) {
            depth++;
        }

        print_node(n);

        if(n -> nodetype != FDEF_T && n -> nodetype != FOR_T && n -> nodetype != IF_T){
            printf(";\n");
        }

        n = n->next;
    }
}

/* Funzione per printare liste di nodi */
void print_list(struct AstNode *l) {
    while(l){
        print_node(l);
        l = l->next;

        if(l){
            printf(", ");
        }
    }
}

/* Funzione per printare le dichiarazioni multiple come singole dichiarazioni */
void print_decl(struct AstNode *l, char * type){
    /* splitta le dichiarazioni multiple , in quanto in golang non possiamo averle con assegnazione se non sono tutte con
    assegnazione, es int a, b=0, c; */
    while(l){
        print_tab(depth);
        printf("%s ", type);
        print_node(l);
        l = l -> next;
        if(l){
            printf(" ; \n");
        }
    }
}
