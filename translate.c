#include "translate.h"
#include "global.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "ast.h"
#include "pretty.h"
#include "semantic.h"
#include "symtab.h"

FILE *output_fp;
FILE *output_fp_h;
int translate_depth = 0;
int scope_lvl = 0;
int table_field_counter = 0;

// Converte un LUA_TYPE nel corrispondente tipo stringa C
const char *lua_type_to_c_string(enum LUA_TYPE type)
{
    switch (type)
    {
    case INT_T:
        return "int";
    case FLOAT_T:
        return "float";
    case STRING_T:
        return "char*";
    case BOOLEAN_T:
        return "bool";
    case TRUE_T:
        return "bool";
    case FALSE_T:
        return "bool";
    case NIL_T:
        return "void*";
    case NUMBER_T:
        return "float";
    case TABLE_T:
        return "lua_field";
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
            // Per gli altri tipi; comprende int, float e boolean
            fprintf(output_fp, "%s", n->node.val->string_val);
            break;
        }
        break;
    case VAR_T:
        if (n->node.var && n->node.var->name)
        {
            fprintf(output_fp, "%s", n->node.var->name);
            // Qui non dichiariamo il tipo, assumiamo sia già stata dichiarata
            // o che il contesto (es. chiamata a funzione) non richieda il tipo.
        }
        else
        {
            fprintf(output_fp, "/* null_variable_name */");
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
        else if (n->node.expr->expr_type == OR_T)
        {
            translate_node(n->node.expr->l, current_scope);
            fprintf(output_fp, " || ");
            translate_node(n->node.expr->r, current_scope);
        }
        else if (n->node.expr->expr_type == ASS_T)
        {
            // Per le assegnazioni, tratta il lato sinistro diversamente,
            // bisogna assegnare il tipo se è la prima dichiarazione
            if (n->node.expr->l && n->node.expr->l->nodetype == VAR_T)
            {
                char *varname = n->node.expr->l->node.var->name;

                // Controlla se la variabile è già stata dichiarata
                struct symbol *sym = find_symtab(current_scope, n->node.expr->l->node.var->name);
                if (sym)
                {
                    // Se è già stata vista prima, è un'assegnazione e non una dichiarazione
                    // Qui includiamo il tipo solo alla prima assegnazione
                    if (sym->used_flag == 0)
                    {
                        // Prima volta che vediamo questa variabile in un'assegnazione
                        fprintf(output_fp, "%s %s", lua_type_to_c_string(sym->type), varname);
                        if (sym->type == TABLE_T)
                        {
                            fprintf(output_fp, "[] ");
                        }
                        // Segna la variabile come usata
                        sym->used_flag = 1;
                    }
                    else
                    {
                        // Variabile già usata, solo nome
                        fprintf(output_fp, "%s", varname);
                    }
                }
                // Se non è stata dichiarata, la dichiariamo ora
                else
                {
                    // È la prima dichiarazione, aggiungi il tipo
                    enum LUA_TYPE type = eval_expr_type(n->node.expr->r, current_scope).type;
                    fprintf(output_fp, "%s %s", lua_type_to_c_string(type), varname);

                    // Aggiungi il simbolo alla tabella simboli durante la traduzione
                    if (current_scope)
                    {
                        insert_sym(current_scope, varname, type, VARIABLE, NULL, 0, "");
                        sym = find_symtab(current_scope, varname);
                        if (sym)
                        {
                            sym->used_flag = 1;
                        }
                    }
                }
            }
            else
            {
                fprintf(output_fp, "/* unknown variable */");
            }
            fprintf(output_fp, " = ");
            translate_node(n->node.expr->r, current_scope);
        }
        else
        {
            // ADD_T, SUB_T, DIV_T, MUL_T,
            // G_T, GE_T, L_T, LE_T, EQ_T, NE_T
            if (n->node.expr->l)
            {
                translate_node(n->node.expr->l, current_scope);
            }
            // La funzione convert_expr_type restituisce il simbolo C corretto per i vari operatori
            // tranne che per NE_T che in lua è "~=" mentre in C è "!="
            const char *c_operator;
            if (n->node.expr->expr_type == NE_T)
            {
                c_operator = "!=";
            }
            else
            {
                c_operator = convert_expr_type(n->node.expr->expr_type);
            }
            fprintf(output_fp, " %s ", c_operator);

            if (n->node.expr->r)
            {
                // Operando destro
                translate_node(n->node.expr->r, current_scope);
            }
        }
        break;
    case IF_T:
        fprintf(output_fp, "if (");
        translate_node(n->node.ifn->cond, current_scope);
        fprintf(output_fp, ") {\n");

        translate_depth++;

        // Recupera la tabella dello scope corrente
        scope_lvl++;
        struct symlist *then_scope = create_symtab(scope_lvl, current_scope);

        // Traduci il corpo del 'then' con il nuovo scope
        struct AstNode *then_body = n->node.ifn->body;
        while (then_body)
        {
            translate_tab();
            translate_node(then_body, then_scope);

            if (then_body->nodetype != FDEF_T && then_body->nodetype != FOR_T && then_body->nodetype != IF_T)
            {
                fprintf(output_fp, ";\n");
            }
            then_body = then_body->next;
        }

        delete_symtab(then_scope);
        scope_lvl--;

        translate_depth--;

        translate_tab();
        fprintf(output_fp, "}");

        if (n->node.ifn->else_body)
        {
            fprintf(output_fp, " else {\n");
            translate_depth++;

            // Recupera la tabella dello scope corrente
            scope_lvl++;
            struct symlist *else_scope = create_symtab(scope_lvl, current_scope);

            struct AstNode *else_body = n->node.ifn->else_body;
            while (else_body)
            {
                translate_tab();
                translate_node(else_body, else_scope);

                if (else_body->nodetype != FDEF_T && else_body->nodetype != FOR_T && else_body->nodetype != IF_T)
                {
                    fprintf(output_fp, ";\n");
                }
                else_body = else_body->next;
            }

            delete_symtab(else_scope);
            scope_lvl--;

            translate_depth--;
            translate_tab();
            fprintf(output_fp, "}");
        }
        fprintf(output_fp, "\n");
        break;
    case FOR_T:
        fprintf(output_fp, "for (");

        fprintf(output_fp, "int %s = ", n->node.forn->varname);
        if (n->node.forn->start)
        {
            translate_node(n->node.forn->start, current_scope);
        }
        else
        {
            fprintf(output_fp, "0");
        }

        fprintf(output_fp, "; %s <= ", n->node.forn->varname);

        // Condizione finale del ciclo
        if (n->node.forn->end)
        {
            translate_node(n->node.forn->end, current_scope);
        }
        else
        {
            fprintf(output_fp, "0");
        }

        fprintf(output_fp, "; ");

        if (n->node.forn->step)
        {
            fprintf(output_fp, "%s += ", n->node.forn->varname);
            translate_node(n->node.forn->step, current_scope);
        }
        else
        {
            fprintf(output_fp, "%s++", n->node.forn->varname); // Default step è 1
        }

        fprintf(output_fp, ") {\n");

        translate_depth++;

        scope_lvl++;
        struct symlist *for_scope = create_symtab(scope_lvl, current_scope);

        insert_sym(for_scope, n->node.forn->varname, INT_T, VARIABLE, NULL, 0, "");

        struct AstNode *for_body = n->node.forn->stmt;
        while (for_body)
        {
            translate_tab();
            translate_node(for_body, for_scope);

            if (for_body->nodetype != FDEF_T && for_body->nodetype != FOR_T && for_body->nodetype != IF_T)
            {
                fprintf(output_fp, ";\n");
            }
            for_body = for_body->next;
        }

        delete_symtab(for_scope);
        scope_lvl--;

        translate_depth--;

        translate_tab();
        fprintf(output_fp, "}\n");
        break;

    case TABLE_NODE_T:
        fprintf(output_fp, "{");
        if (n->node.table)
        {
            translate_depth++;
            struct AstNode *field = n->node.table->fields;

            // Caso tabella vuota senza campi
            if (!field)
            {
                translate_depth--;
                fprintf(output_fp, " }");
                break;
            }

            // Caso tabella con un solo campo vuoto
            if (field && field->next == NULL &&
                ((field->nodetype != TABLE_FIELD_T) ||
                 (field->nodetype != VAL_T)))
            {
                translate_depth--;
                fprintf(output_fp, " }");
                break;
            }

            table_field_counter = 0;

            // Caso tabella con un campo
            if (field && field->next == NULL)
            {
                // Niente virgola, solo un campo
                translate_tab();
                translate_node(field, current_scope);
                translate_depth--;
                fprintf(output_fp, "\n");
                translate_tab();
                fprintf(output_fp, "}");
                break;
            }

            fprintf(output_fp, "\n");

            // Caso tabella con più campi
            while (field)
            {
                translate_tab();
                translate_node(field, current_scope);
                fprintf(output_fp, ",\n");
                field = field->next;
            }
            translate_depth--;
            translate_tab();
        }
        fprintf(output_fp, "}");
        break;
    case TABLE_FIELD_T:
        if (n->node.tfield)
        {
            if (n->node.tfield->key)
            {
                // Se c'è una chiave, stampala
                fprintf(output_fp, "{ \"");
                translate_node(n->node.tfield->key, current_scope);
                fprintf(output_fp, "\", ");
            }
            else
            {
                // Genera automaticamente una chiave per i campi senza chiave
                fprintf(output_fp, "{ \"key_%d\", ", table_field_counter++);
            }

            // Determina il tipo del valore
            enum LUA_TYPE type = NIL_T;

            // Se il valore è un VAL_T, usa il suo tipo direttamente
            if (n->node.tfield->value && n->node.tfield->value->nodetype == VAL_T)
            {
                type = n->node.tfield->value->node.val->val_type;
            }
            else
            {
                // Per le espressioni, infera il tipo
                type = eval_expr_type(n->node.tfield->value, current_scope).type;
            }

            switch (type)
            {
            case STRING_T:
                fprintf(output_fp, "{.string_value = ");
                translate_node(n->node.tfield->value, current_scope);
                fprintf(output_fp, "}");
                break;
            case INT_T:
                fprintf(output_fp, "{.int_value = ");
                translate_node(n->node.tfield->value, current_scope);
                fprintf(output_fp, "}");
                break;
            case FLOAT_T:
            case NUMBER_T:
                fprintf(output_fp, "{.float_value = ");
                translate_node(n->node.tfield->value, current_scope);
                fprintf(output_fp, "}");
                break;
            case FALSE_T:
                fprintf(output_fp, "{.bool_value = ");
                translate_node(n->node.tfield->value, current_scope);
                fprintf(output_fp, "}");
                break;
            case TRUE_T:
                fprintf(output_fp, "{.bool_value = ");
                translate_node(n->node.tfield->value, current_scope);
                fprintf(output_fp, "}");
                break;
            default:
                // Fallback a intero come default
                fprintf(output_fp, "{.int_value = ");
                translate_node(n->node.tfield->value, current_scope);
                fprintf(output_fp, "} /* default type */");
                break;
            }
            fprintf(output_fp, " }");
        }
        else
        {
            // Fallback per campi senza chiave o valore
            fprintf(output_fp, "{ \"key_%d\", {.int_value = 0} }", table_field_counter++);
        }
        break;
    case RETURN_T:
        fprintf(output_fp, "return");
        if (n->node.ret->expr)
        {
            fprintf(output_fp, " ");
            translate_node(n->node.ret->expr, current_scope);
            if (n->node.ret->expr->next)
            {
                fprintf(output_fp,
                        " /* Lua multiple return values not directly supported in C, only first value translated */");
            }
        }
        break;
    case FCALL_T:
        // Gestione per print
        if (n->node.fcall->func_expr->nodetype == VAR_T &&
            n->node.fcall->func_expr->node.var->name != NULL &&
            strcmp(n->node.fcall->func_expr->node.var->name, "print") == 0)
        {
            struct AstNode *arg = n->node.fcall->args;

            if (!arg)
            {
                // print()
                fprintf(output_fp, "printf(\"\\n\")"); // Lua stampa una nuova riga
            }
            else
            {
                fprintf(output_fp, "printf(\"");
                // Fase 1: Costruire la stringa di formato
                struct AstNode *current_arg_for_format = arg;
                bool first_item_in_format = true;
                while (current_arg_for_format)
                {
                    if (!first_item_in_format)
                    {
                        fprintf(output_fp, " ");
                    }

                    struct complex_type ct = eval_expr_type(current_arg_for_format, current_scope);
                    enum LUA_TYPE type_of_arg = ct.type;

                    // Controllo se l'argomento è un VALORE STRINGA LETTERALE
                    if (current_arg_for_format->nodetype == VAL_T &&
                        current_arg_for_format->node.val->val_type == STRING_T)
                    {
                        fprintf(output_fp, "%s", current_arg_for_format->node.val->string_val);
                    }
                    else
                    {
                        if (current_arg_for_format->nodetype == VAL_T)
                        {
                            type_of_arg = current_arg_for_format->node.val->val_type;
                        }

                        switch (type_of_arg)
                        {
                        case INT_T:
                            fprintf(output_fp, "%%d");
                            break;
                        case FLOAT_T:
                            fprintf(output_fp, "%%f");
                            break;
                        case NUMBER_T:
                            fprintf(output_fp, "%%g");
                            break;
                        case STRING_T:
                            fprintf(output_fp, "%%s");
                            break;
                        case NIL_T:
                            fprintf(output_fp, "NULL");
                            break;
                        case TRUE_T:
                            fprintf(output_fp, "true");
                            break;
                        case FALSE_T:
                            fprintf(output_fp, "false");
                            break;
                        case BOOLEAN_T:
                            fprintf(output_fp, "%%s");
                            break;
                        case FUNCTION_T:
                            fprintf(output_fp, "function");
                            break;
                        case TABLE_T:
                            fprintf(output_fp, "table");
                            break;
                        case USERDATA_T:
                            fprintf(output_fp, "userdata");
                            break;
                        default:
                            fprintf(output_fp, "%%s");
                            break;
                        }
                    }
                    first_item_in_format = false;
                    current_arg_for_format = current_arg_for_format->next;
                }
                fprintf(output_fp, "\\n\""); // Aggiungere newline e chiudere la stringa di formato

                // Fase 2: Aggiungere gli argomenti alla chiamata printf
                struct AstNode *current_arg_for_value = arg;
                bool needs_comma = false;
                while (current_arg_for_value)
                {
                    struct complex_type ct = eval_expr_type(current_arg_for_value, current_scope);
                    enum LUA_TYPE type_of_arg = ct.type;

                    bool is_literal_string = (current_arg_for_value->nodetype == VAL_T &&
                                              current_arg_for_value->node.val->val_type == STRING_T);

                    if (!is_literal_string)
                    {
                        // Solo se non è un letterale stringa
                        if (current_arg_for_value->nodetype == VAL_T)
                        {
                            type_of_arg = current_arg_for_value->node.val->val_type;
                        }

                        switch (type_of_arg)
                        {
                        case INT_T:
                        case FLOAT_T:
                        case NUMBER_T:
                        case STRING_T:
                            if (needs_comma)
                                fprintf(output_fp, ", ");
                            else
                                fprintf(output_fp, ", "); // Stampare virgola se c'è un argomento
                            translate_node(current_arg_for_value, current_scope);
                            needs_comma = true;
                            break;
                        case BOOLEAN_T:
                            if (needs_comma)
                                fprintf(output_fp, ", ");
                            else
                                fprintf(output_fp, ", ");
                            fprintf(output_fp, "(");
                            translate_node(current_arg_for_value, current_scope);
                            fprintf(output_fp, ") ? \"true\" : \"false\"");
                            needs_comma = true;
                            break;
                        default:
                            break;
                        }
                    }
                    current_arg_for_value = current_arg_for_value->next;
                }
                fprintf(output_fp, ")");
            }
        }
        // Gestione per io.read
        else if (n->node.fcall->func_expr->nodetype == VAR_T &&
                 n->node.fcall->func_expr->node.var->name != NULL &&
                 strcmp(n->node.fcall->func_expr->node.var->name, "io.read") == 0)
        {
            struct AstNode *arg1 = n->node.fcall->args;
            if (!arg1)
            {
                // io.read() di default è "*l"
                fprintf(output_fp, "c_lua_io_read_line()");
            }
            else
            {
                if (arg1->nodetype == VAL_T && arg1->node.val->val_type == STRING_T)
                {
                    const char *fmt = arg1->node.val->string_val;
                    if (strcmp(fmt, "*n") == 0)
                    {
                        fprintf(output_fp, "c_lua_io_read_number()");
                    }
                    else if (strcmp(fmt, "*l") == 0 || strcmp(fmt, "*L") == 0)
                    {
                        // *L è come *l
                        fprintf(output_fp, "c_lua_io_read_line()");
                    }
                    else if (strcmp(fmt, "*a") == 0)
                    {
                        fprintf(output_fp,
                                "/* io.read(\"*a\") - read all; complex, using simplified line read */ c_lua_io_read_line()");
                    }
                    else
                    {
                        fprintf(output_fp, "io_read_unsupported_format(\"%s\")", fmt);
                    }
                }
                else if (arg1->nodetype == VAL_T && (arg1->node.val->val_type == INT_T || arg1->node.val->val_type ==
                                                                                              FLOAT_T))
                {
                    fprintf(output_fp, "c_lua_io_read_bytes(");
                    translate_node(arg1, current_scope);
                    fprintf(output_fp, ")");
                }
                else
                {
                    fprintf(output_fp, "io_read_complex_arg()");
                }
                if (arg1->next)
                {
                    fprintf(output_fp, " /* , ... further arguments to io.read ignored */");
                }
            }
        }
        else if (n->node.fcall->func_expr->nodetype == VAR_T)
        {
            // Normale chiamata a funzione
            if (n->node.fcall->func_expr->node.var->name != NULL)
            {
                fprintf(output_fp, "%s", n->node.fcall->func_expr->node.var->name);
            }
            else
            {
                fprintf(output_fp, "/* anonymous_or_null_fcall_var_name */");
            }
            fprintf(output_fp, "(");
            if (n->node.fcall->args)
            {
                translate_list(n->node.fcall->args, ", ");
            }
            fprintf(output_fp, ")");
        }
        break;

    case FDEF_T:
        // Funzione definita dall'utente
        if (n->node.fdef->ret_type)
        {
            // Se la funzione ha un tipo di ritorno, lo indichiamo
            fprintf(output_fp, "%s ", lua_type_to_c_string(n->node.fdef->ret_type));
        }
        else
        {
            fprintf(output_fp, "void "); // Funzione senza tipo di ritorno
        }

        if (n->node.fdef->name)
        {
            fprintf(output_fp, "%s(", n->node.fdef->name);
            if (n->node.fdef->params)
            {
                translate_params(n->node.fdef->params, current_scope);
            }
            fprintf(output_fp, ") {\n");

            translate_depth++;

            scope_lvl++;
            struct symlist *func_scope = create_symtab(scope_lvl, current_scope);

            // Aggiungi gli argomenti alla tabella dei simboli con i tipi appropriati
            struct AstNode *arg = n->node.fdef->params;
            while (arg)
            {
                if (arg->nodetype == VAR_T && arg->node.var && arg->node.var->name)
                {
                    // Cerca nel contesto delle chiamate di funzione per inferire il tipo del parametro
                    enum LUA_TYPE param_type = INT_T; // Tipo predefinito

                    // Cerca il parametro nella tabella dei simboli del parent scope (se esiste)
                    if (current_scope && current_scope->next)
                    {
                        struct symbol *func_sym = find_sym(current_scope->next, n->node.fdef->name);
                        if (func_sym && func_sym->pl)
                        {
                            // Cerca il parametro corrispondente nelle chiamate di funzione
                            struct AstNode *func_call_args = func_sym->pl;
                            struct AstNode *current_arg = func_call_args;
                            int param_index = 0;
                            int current_index = 0;

                            // Calcola l'indice di questo parametro
                            struct AstNode *temp = n->node.fdef->params;
                            while (temp && temp != arg)
                            {
                                param_index++;
                                temp = temp->next;
                            }

                            // Trova l'argomento corrispondente nella chiamata di funzione
                            while (current_arg && current_index < param_index)
                            {
                                current_index++;
                                current_arg = current_arg->next;
                            }

                            // Se troviamo l'argomento, usa il suo tipo
                            if (current_arg)
                            {
                                param_type = eval_expr_type(current_arg, current_scope).type;
                            }
                        }
                    }

                    // Inserisci il parametro con il tipo inferito
                    insert_sym(func_scope, arg->node.var->name, param_type, PARAMETER, NULL, 0, "");
                }
                else if (arg->nodetype == DECL_T && arg->node.decl->var &&
                         arg->node.decl->var->nodetype == VAR_T)
                {
                    // Parametro con valore di default
                    enum LUA_TYPE param_type = eval_expr_type(arg->node.decl->expr, func_scope).type;
                    insert_sym(func_scope, arg->node.decl->var->node.var->name,
                               param_type, PARAMETER, NULL, 0, "");
                }
                arg = arg->next;
            }

            // Traduci il corpo della funzione
            struct AstNode *body = n->node.fdef->code;
            while (body)
            {
                translate_tab();
                translate_node(body, func_scope);

                if (body->nodetype != FDEF_T && body->nodetype != FOR_T && body->nodetype != IF_T)
                {
                    fprintf(output_fp, ";\n");
                }
                body = body->next;
            }

            delete_symtab(func_scope);
            scope_lvl--;

            translate_depth--;

            translate_tab();
            fprintf(output_fp, "}\n");
        }
        else
        {
            fprintf(output_fp, "/* anonymous function */");
        }
        break;
    default:
        fprintf(output_fp, "/* unknown expression */");
        break;
    }
}

// Funzione per tradurre una lista di parametri di funzione con i loro tipi
void translate_params(struct AstNode *params, struct symlist *current_symtab)
{
    bool first = true;
    while (params)
    {
        if (!first)
        {
            fprintf(output_fp, ", ");
        }

        // Gestione dei parametri in base al loro tipo
        if (params->nodetype == VAR_T && params->node.var && params->node.var->name)
        {
            // Trova il tipo del parametro nella tabella dei simboli
            // Se non lo troviamo, assumiamo che sia un intero
            enum LUA_TYPE param_type = INT_T;

            struct symbol *param_sym = find_symtab(current_symtab, params->node.var->name);
            if (param_sym)
            {
                param_type = param_sym->type;
            }

            fprintf(output_fp, "%s %s", lua_type_to_c_string(param_type), params->node.var->name);
        }
        else if (params->nodetype == DECL_T && params->node.decl->var &&
                 params->node.decl->var->nodetype == VAR_T)
        {
            // Parametro con valore di default
            // Trova il tipo del parametro dalla dichiarazione
            enum LUA_TYPE param_type = eval_expr_type(params->node.decl->expr, current_symtab).type;
            fprintf(output_fp, "%s %s", lua_type_to_c_string(param_type),
                    params->node.decl->var->node.var->name);
        }
        else
        {
            // Fallback per parametri non riconosciuti
            fprintf(output_fp, "void* param%d", first ? 1 : 0);
        }

        first = false;
        params = params->next;
    }
}

// Funzione per generare il prototipo di funzione nell'header
void generate_func_prototype(struct AstNode *func_node)
{
    if (!func_node || func_node->nodetype != FDEF_T || !output_fp_h)
        return;

    // Generiamo il tipo di ritorno
    if (func_node->node.fdef->ret_type)
    {
        fprintf(output_fp_h, "%s ", lua_type_to_c_string(func_node->node.fdef->ret_type));
    }
    else
    {
        fprintf(output_fp_h, "void ");
    }

    // Nome della funzione
    if (func_node->node.fdef->name)
    {
        fprintf(output_fp_h, "%s(", func_node->node.fdef->name);

        // Parametri
        if (func_node->node.fdef->params)
        {
            translate_params(func_node->node.fdef->params, root_symtab);
        }

        fprintf(output_fp_h, ");\n");
    }
}

void translate(struct AstNode *root_ast_node)
{
    printf(">> Inizio traduzione da Lua a C...\n");

    // Costruzione del nome del file di output
    char *output_filename_base = NULL;
    char *output_filename_c = NULL;
    char *output_filename_h = NULL;

    if (filename)
    {
        // Trova l'ultima occorrenza di '.' per rimuovere l'estensione .lua
        char *dot_position = strrchr(filename, '.');
        if (dot_position != NULL)
        {
            // Calcola la lunghezza della base del nome del file
            size_t base_len = dot_position - filename;
            output_filename_base = (char *)malloc(base_len + 1);
            if (output_filename_base)
            {
                strncpy(output_filename_base, filename, base_len);
                output_filename_base[base_len] = '\0';
            }
        }
        else
        {
            // Nessuna estensione trovata, usa l'intero nome del file come base
            output_filename_base = strdup(filename);
        }

        if (output_filename_base)
        {
            size_t c_filename_len = strlen(output_filename_base) + 2 + 1;
            output_filename_c = (char *)malloc(c_filename_len);
            output_filename_h = (char *)malloc(c_filename_len);
            if (output_filename_c)
            {
                sprintf(output_filename_c, "%s.c", output_filename_base);
            }
            if (output_filename_h)
            {
                sprintf(output_filename_h, "%s.h", output_filename_base);
            }
            free(output_filename_base);
        }
    }

    // Fallback se la costruzione del nome fallisce o filename è NULL
    if (!output_filename_c)
    {
        fprintf(
            stderr,
            YELLOW "ATTENZIONE:" RESET
                   " Impossibile derivare il nome del file di output dal sorgente. Uso 'output.c' come default.\n");
        output_filename_c = strdup("output.c");
        if (!output_filename_c)
        {
            fprintf(stderr, RED "ERRORE:" RESET " Fallimento critico nell'allocazione del nome del file di output.\n");
            exit(1);
        }
    }

    // Apri il file di output
    output_fp = fopen(output_filename_c, "w");

    if (!output_fp)
    {
        fprintf(stderr, RED "ERRORE:" RESET " Impossibile aprire il file di output C '%s'.\n", output_filename_c);
        perror("fopen");
        free(output_filename_c);
        exit(1);
    }

    char *header_filename = (char *)malloc(strlen(output_filename_h) + 1);
    header_filename = strrchr(output_filename_h, '/');
    if (header_filename)
    {
        header_filename++; // Skippa il / se lo trova
    }
    else
    {
        header_filename = output_filename_h; // Usa tutta la stringa se non trova /
    }
    fprintf(output_fp, "#include \"%s\"\n\n", header_filename);

    // Traduzione le definizioni di funzione Lua PRIMA del main
    struct AstNode *current_node = root_ast_node;
    while (current_node)
    {
        if (current_node->nodetype == FDEF_T)
        {
            translate_node(current_node, root_symtab); // Passa la symbol table globale
        }
        current_node = current_node->next;
    }

    // Inizio della funzione main() C
    fprintf(output_fp, "int main() {\n");
    translate_depth++;

    // Traduzione degli statement globali Lua (che non sono FDEF_T) dentro main()
    current_node = root_ast_node;

    scope_lvl = 0;

    while (current_node)
    {
        if (current_node->nodetype != FDEF_T)
        {
            // Salta le definizioni di funzione, già tradotte
            translate_tab(); // Indenta lo statement corrente
            translate_node(current_node, root_symtab);
            if (current_node->nodetype != IF_T && current_node->nodetype != FOR_T)
            {
                fprintf(output_fp, ";\n");
            }
        }
        current_node = current_node->next;
    }

    // Fine della funzione main() C
    translate_tab();
    fprintf(output_fp, "return 0;\n");
    translate_depth--;
    fprintf(output_fp, "}\n");

    // Chiudi il file di output
    fclose(output_fp);
    printf(">> Traduzione completata. Codice C generato in '%s'.\n", output_filename_c);

    // Defnizione header
    printf(">> Generazione del file header...\n");
    output_fp_h = fopen(output_filename_h, "w");

    // include C necessari all'inizio del file
    fprintf(output_fp, "#include <stdio.h>\n");
    fprintf(output_fp, "#include <stdlib.h>\n");
    fprintf(output_fp, "#include <stdbool.h>\n");
    fprintf(output_fp_h, "char* c_lua_io_read_line(){\n \
    char *buff;\n\
    scanf(\"%%ms\", &buff);\n\
    return buff;\n\
}\n\n");

    fprintf(output_fp_h, "float c_lua_io_read_number(){\n \
    float ret;\n\
    scanf(\"%%f\", &ret);\n\
    return ret;\n\
}\n\n");

    fprintf(output_fp_h, "char *c_lua_io_read_bytes(int n)\n\
{\n\
    char *buff = (char *)malloc(sizeof(char) * (n + 1));\n\
    scanf(\"%%ms\", &buff);\n\
    buff[n] = \'\\0\';\n\
    return buff;\n\
}\n\n");

    fprintf(output_fp_h, "typedef struct\n\
{\n\
    char *key;\n\
    union value\n\
        {\n\
            int int_value;\n\
            double float_value;\n\
            char *string_value;\n\
            bool bool_value;\n\
        } value;\n\
} lua_field;\n\n");

    // Genera i prototipi delle funzioni nell'header
    current_node = root_ast_node;
    while (current_node)
    {
        if (current_node->nodetype == FDEF_T)
        {
            generate_func_prototype(current_node);
        }
        current_node = current_node->next;
    }
    printf(">> Header completo in '%s'.\n", output_filename_h);
    fclose(output_fp_h);
    free(output_filename_c);
}
