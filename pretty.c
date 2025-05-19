#include "pretty.h"
#include "global.h"
#include <stdlib.h>
#include <stdio.h>

int depth = 0;
/* Funzione per stampare il numero di tabulazioni */
void print_tab(int depth)
{
    for (int i = 0; i < depth; i++)
    {
        printf("\t");
    }
}
/* Funzione per printare i nodi Ast */
void print_node(struct AstNode *n)
{
    switch (n->nodetype)
    {
    case VAL_T:
        if (n->node.val->val_type == STRING_T)
        {
            printf("\"");
            printf("%s", n->node.val->string_val);
            printf("\"");
        }
        else
        {
            printf("%s", n->node.val->string_val);
        }
        break;
    case VAR_T:
        printf("%s", n->node.var->name);
        if (n->node.var->table_key)
        {
            printf("[");
            print_node(n->node.var->table_key);
            printf("]");
        }
        break;
    case DECL_T:
        print_list(n->node.decl->var);
        break;
    case EXPR_T:
        if (n->node.expr->expr_type == PAR_T)
        {
            printf("(");
            print_node(n->node.expr->r);
            printf(")");
        }
        else
        {
            if (n->node.expr->l)
                print_node(n->node.expr->l);
            printf("%s", convert_expr_type(n->node.expr->expr_type));
            print_node(n->node.expr->r);
        }
        break;
    case RETURN_T:
        printf("return ");
        if (n->node.ret->expr)
        {
            print_node(n->node.ret->expr);
        }
        break;
    case FCALL_T:
        if (n->node.fcall->func_expr->nodetype == VAR_T)
        {
            printf("%s(", n->node.fcall->func_expr->node.var->name);
        }
        else
        {
            print_node(n->node.fcall->func_expr);
            printf("(");
        }
        if (n->node.fcall->args)
        {
            print_list(n->node.fcall->args);
        }
        printf(")");
        break;
    case FDEF_T:
        printf("%s(", n->node.fdef->name);
        if (n->node.fdef->params)
        {
            print_list(n->node.fdef->params);
        }
        printf(") {\n");
        print_ast(n->node.fdef->code);
        depth--;
        print_tab(depth);
        printf("}\n\n");
        break;
    case FOR_T:
        printf("for %s = ", n->node.forn->varname);
        print_node(n->node.forn->start);
        printf(", ");
        print_node(n->node.forn->end);
        if (n->node.forn->step)
        {
            printf(", ");
            print_node(n->node.forn->step);
        }
        printf(" do\n");
        print_ast(n->node.forn->stmt);
        depth--;
        print_tab(depth);
        printf("end\n");
        break;
    case IF_T:
        printf("if(");
        print_node(n->node.ifn->cond);
        printf(") {\n");
        print_ast(n->node.ifn->body);
        depth--;
        print_tab(depth);
        printf("}");
        if (n->node.ifn->else_body)
        {
            printf("else{\n");
            depth++;
            print_ast(n->node.ifn->else_body);
            depth--;
            print_tab(depth);
            printf("}\n");
        }
        else
        {
            printf("\n");
        }
        break;
    case TABLE_NODE_T:
        printf("{");
        if (n->node.table->fields)
        {
            print_list(n->node.table->fields);
        }
        printf("}");
        break;
    case TABLE_FIELD_T:
        if (n->node.tfield->key)
        {
            print_node(n->node.tfield->key);
            if (n->node.tfield->value != NULL)
            {
                printf(" = ");
                print_node(n->node.tfield->value);
            }
        }
        else
        {
            printf("nil");
        }
        break;
    case ERROR_NODE_T:
        printf("error node");
    }
}

/* Funzione per printare l'Ast */
void print_ast(struct AstNode *n)
{
    while (n)
    {
        print_tab(depth);

        if (n->nodetype == FDEF_T || n->nodetype == FOR_T || n->nodetype == IF_T)
        {
            depth++;
        }

        print_node(n);

        if (n->nodetype != FDEF_T && n->nodetype != FOR_T && n->nodetype != IF_T)
        {
            printf("\n");
        }

        n = n->next;
    }
}

/* Funzione per printare liste di nodi */
void print_list(struct AstNode *l)
{
    while (l)
    {
        print_node(l);
        l = l->next;

        if (l)
        {
            printf(", ");
        }
    }
}

/* Funzione per printare le dichiarazioni multiple come singole dichiarazioni */
void print_decl(struct AstNode *l, char *type)
{
    /* splitta le dichiarazioni multiple , in quanto in golang non possiamo averle con assegnazione se non sono tutte con
    assegnazione, es int a, b=0, c; */
    while (l)
    {
        print_tab(depth);
        printf("%s ", type);
        print_node(l);
        l = l->next;
        if (l)
        {
            printf(" ; \n");
        }
    }
}

/* Convertire un tipo di espressione in una stringa */
char *convert_expr_type(enum EXPRESSION_TYPE expr_type)
{
    switch (expr_type)
    {
    case ADD_T:
        return "+";
    case SUB_T:
        return "-";
    case MUL_T:
        return "*";
    case DIV_T:
        return "/";
    case NOT_T:
        return "not";
    case AND_T:
        return "and";
    case OR_T:
        return "or";
    case G_T:
        return ">";
    case GE_T:
        return ">=";
    case L_T:
        return "<";
    case LE_T:
        return "<=";
    case EQ_T:
        return "==";
    case NE_T:
        return "~=";
    case ASS_T:
        return "=";
    case NEG_T:
        return "-";
    case PAR_T:
        return "()";
    default:
        return "unknown";
    }
}

// Aggiungere la funzione convert_var_type
char *convert_var_type(enum LUA_TYPE type)
{
    switch (type)
    {
    case NIL_T:
        return "nil";
    case TRUE_T:
        return "true";
    case FALSE_T:
        return "false";
    case NUMBER_T:
        return "number";
    case INT_T:
        return "integer";
    case FLOAT_T:
        return "float";
    case STRING_T:
        return "string";
    case FUNCTION_T:
        return "function";
    case TABLE_T:
        return "table";
    case USERDATA_T:
        return "userdata";
    case ERROR_T:
        return "error";
    default:
        return "unknown";
    }
}

/* Funzione per convertire il tipo di nodo */
// usata per debug
char *convert_node_type(enum NODE_TYPE t)
{
    switch (t)
    {
    case EXPR_T:
        return "expr node";
    case IF_T:
        return "if node";
    case VAL_T:
        return "val node";
    case VAR_T:
        return "var node";
    case DECL_T:
        return "decl node";
    case RETURN_T:
        return "ret node";
    case FCALL_T:
        return "func call node";
    case FDEF_T:
        return "func def node";
    case FOR_T:
        return "for node";
    case ERROR_NODE_T:
        return "error node";
    default:
        return "unknown node";
    }
}

/* Funzione per convertire gli identificatori delle funzioni per I/O */
char *convert_func_name(char *name)
{
    if (!strcmp("printf", name))
        return "fmt.Printf";
    if (!strcmp("scanf", name))
        return "fmt.Scanf";
    return name;
}