#include "translate.h"
#include "global.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "ast.h"
#include "pretty.h"
#include "semantic.h"
#include "symtab.h"

FILE *output_fp;         // File pointer per l'output C
int translate_depth = 0; // Per l'indentazione
int scope_lvl = 0;       // Per la gestione degli scope

// Converte un LUA_TYPE nel corrispondente tipo stringa C
const char *lua_type_to_c_string(enum LUA_TYPE type)
{
    switch (type)
    {
    case INT_T:
        return "int";
    case FLOAT_T:
        return "double";
    case STRING_T:
        return "char*";
    case BOOLEAN_T:
        return "bool";
    case TRUE_T:
        return "bool";
    case FALSE_T:
        return "bool";
    case NIL_T:
        return "/* nil_val_in_c */ void*"; // da vedere
    case FUNCTION_T:
        return "/* func_ptr_t */ void*";
    // case USERDATA_T: return "/* userdata_t */ void*";
    case NUMBER_T:
        return "double";
    default:
        return "/* unknown_type */ void*";
    }
}

// Stampa l'indentazione
void translate_tab()
{
    for (int i = 0; i < translate_depth; i++)
    {
        fprintf(output_fp, "    "); // Usa 4 spazi per tab
    }
}

// Funzione per tradurre una lista di argomenti o espressioni
void translate_list(struct AstNode *l, const char *separator)
{
    bool first = true;
    while (l)
    {
        if (!first)
        {
            fprintf(output_fp, "%s", separator);
        }
        translate_node(l, root_symtab);
        first = false;
        l = l->next;
    }
}

// Funzione per tradurre il nodo con consapevolezza del tipo
void translate_node(struct AstNode *n, struct symlist *current_scope)
{
    if (!n)
        return;

    switch (n->nodetype)
    {
    case VAL_T:
        switch (n->node.val->val_type)
        {
        case STRING_T:
            fprintf(output_fp, "\"%s\"", n->node.val->string_val);
            break;
        case NIL_T:
            fprintf(output_fp, "NULL");
            break;
        default:
            // Per gli altri tipi, comprende int, float e boolean
            fprintf(output_fp, "%s", n->node.val->string_val);
            break;
        }
        break;
    case VAR_T:
        // Cerca il simbolo nella symbol table corrente e stampa il tipo
        if (n->node.var && n->node.var->name)
        {
            struct symbol *sym = find_symtab(current_scope, n->node.var->name);
            if (sym)
            {
                // Se stiamo dichiarando la variabile, stampa il tipo
                fprintf(output_fp, "%s %s", lua_type_to_c_string(sym->type), n->node.var->name);
            }
            else
            {
                // Se il simbolo non è trovato, usa il nome della variabile senza tipo
                fprintf(output_fp, "%s", n->node.var->name);
            }
        }
        else
        {
            fprintf(output_fp, "/* null variable */");
        }
        break;
    case EXPR_T:
        if (n->node.expr->expr_type == PAR_T)
        {
            fprintf(output_fp, "(");
            translate_node(n->node.expr->r, current_scope);
            fprintf(output_fp, ")");
        }
        else if (n->node.expr->expr_type == NEG_T)
        {
            fprintf(output_fp, "-");
            translate_node(n->node.expr->r, current_scope);
        }
        else if (n->node.expr->expr_type == NOT_T)
        {
            fprintf(output_fp, "!");
            translate_node(n->node.expr->r, current_scope);
        }
        else if (n->node.expr->expr_type == AND_T)
        {
            translate_node(n->node.expr->l, current_scope);
            fprintf(output_fp, " && ");
            translate_node(n->node.expr->r, current_scope);
        }
        else if (n->node.expr->expr_type == ASS_T)
        {
            // Per le assegnazioni, tratta il lato sinistro in modo speciale
            if (n->node.expr->l && n->node.expr->l->nodetype == VAR_T)
            {
                // Controlla se la variabile è già stata dichiarata
                struct symbol *sym = find_symtab(current_scope, n->node.expr->l->node.var->name);
                if (sym)
                {
                    // Se è già dichiarata, stampa solo il nome
                    fprintf(output_fp, "%s", n->node.expr->l->node.var->name);
                }
                else
                {
                    // Se è la prima dichiarazione, aggiungi il tipo
                    enum LUA_TYPE type = eval_expr_type(n->node.expr->r).type;
                    fprintf(output_fp, "%s %s", lua_type_to_c_string(type),
                            n->node.expr->l->node.var->name);
                }
            }
            else
            {
                translate_node(n->node.expr->l, current_scope);
            }
            fprintf(output_fp, " = ");
            translate_node(n->node.expr->r, current_scope);
        }
        else
        {
            if (n->node.expr->l)
                translate_node(n->node.expr->l, current_scope);
            fprintf(output_fp, " %s ", convert_expr_type(n->node.expr->expr_type));
            translate_node(n->node.expr->r, current_scope);
        }
        break;
    default:
        fprintf(output_fp, "/* unknown expression */");
        break;
    }
}

/* Funzione per tradurre l'Ast */
void translate_ast(struct AstNode *n)
{
    while (n)
    {
        if (n->nodetype != DECL_T)
        {
            translate_tab(translate_depth);
        }
        if (n->nodetype == FDEF_T || n->nodetype == FOR_T || n->nodetype == IF_T)
        {
            scope_lvl++;
            translate_depth++;
        }

        translate_node(n, root_symtab);

        if (n->nodetype != DECL_T && n->nodetype != FDEF_T && n->nodetype != FOR_T && n->nodetype != IF_T)
        {
            fprintf(output_fp, ";\n");
        }

        n = n->next;
    }
}

void translate(struct AstNode *root)
{
    printf(">> Generated C translation!\n");
    output_fp = fopen("prova.c", "w");

    if (!output_fp)
    {
        fprintf(stderr, RED "error:" RESET " %s: ", filename);
        perror("");
        exit(1);
    }

    // Includi gli header necessari per il codice C generato
    // TODO: da includere solo se necessario
    fprintf(output_fp, "#include <stdio.h>\n");
    fprintf(output_fp, "#include <stdlib.h>\n");
    fprintf(output_fp, "#include <stdbool.h>\n");
    fprintf(output_fp, "#include <string.h>\n\n");

    translate_ast(root);

    fclose(output_fp);
}
