%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "semantic.h"
#include "symtab.h"
#include "global.h"
#include "pretty.h"
#include "translate.h"

extern int yylex();
extern FILE *yyin;
extern int yylineno;
extern char *line;

struct AstNode *root;
struct AstNode *param_list = NULL; // puntatore alla lista dei parametri di una funzione, da inserire nello scope
enum LUA_TYPE ret_type = -1; // tipo di ritorno della funzione, da inserire nello scope

void scope_enter();
void scope_exit();
void fill_symtab (struct symlist * syml, struct AstNode *node, enum LUA_TYPE type, enum sym_type sym_type);

int print_symtab_flag = 0;
int print_ast_flag = 0;
void print_usage();

// void check_var_reference(struct AstNode *var);
void check_fcall(struct AstNode *func_expr, struct AstNode *args);

// Funzione helper per creare l'identificatore per io.read e liberare le stringhe originali
static struct AstNode* new_io_read_identifier_node(char* ns_token, char* func_token) {
    if (strcmp(ns_token, "io") == 0 && strcmp(func_token, "read") == 0) {
        char* full_name = malloc(strlen(ns_token) + 1 + strlen(func_token) + 1);
        if (!full_name) { /* Gestione errore malloc */ perror("malloc failed for io.read id"); exit(EXIT_FAILURE); }
        sprintf(full_name, "%s.%s", ns_token, func_token);

        // new_variable assegna direttamente il puntatore 'full_name'.
        // 'full_name' sarà quindi gestito (eventualmente liberato) insieme al nodo AST.
        struct AstNode* var_node = new_variable(VAR_T, full_name, NULL);

        free(ns_token); // Libera la stringa strdup-pata dal lexer per "io"
        free(func_token); // Libera la stringa strdup-pata dal lexer per "read"
        return var_node;
    }
    // Se non è "io.read", è un errore per questa implementazione semplificata
    yyerror(error_string_format("Unsupported table member access: %s.%s. Only 'io.read' is supported.", ns_token, func_token));
    free(ns_token);
    free(func_token);
    return new_error(ERROR_NODE_T); // Ritorna un nodo errore
}
%}

%define parse.error verbose

%union {
    char* s;
    struct AstNode *ast;
    int t;
}

%token <s> INT_NUM FLOAT_NUM
%token  IF ELSE THEN FOR DO END RETURN
%token  FUNCTION
%token <s> STRING BOOL NIL
%token DOT

%right  '='
%left   OR
%left   AND
%left   '>' '<' GE LE EQ NE
%left   '+' '-'
%left   '*' '/'
%right  NOT

%precedence UMINUS // Per il meno unario
%precedence LOWEST // Livello di precedenza più basso, usato con %prec

%nonassoc <s> ID
%nonassoc '('

%type <ast> number global_statement_list global_statement expr  assignment
%type <ast> return_statement func_call args if_cond
%type <ast> statement_list statement
%type <ast> name_or_ioread
%type <ast> func_definition param_list param table_field table_list chunk
%type <ast> iteration_statement start_expr end_expr step selection_statement selection_ending optional_expr_list
%type <ast> primary_expr

%%

program
    : { scope_enter(); } global_statement_list                        { root = $2; scope_exit(); }
    ;


global_statement_list
    : global_statement
    | global_statement global_statement_list                        { $$ = link_AstNode($1, $2); }
    ;

global_statement
    : assignment
    | func_definition
    | selection_statement
    | iteration_statement
    | func_call
    | return_statement
    ;

 func_definition
     : FUNCTION ID '(' param_list ')'
         { scope_enter(); param_list = $4; } // current_symtab è ora lo scope della funzione
             chunk   END
         {
           // Passa current_symtab (che è lo scope interno della funzione)
           enum LUA_TYPE ret = infer_func_return_type($7, current_symtab);
           $$ = new_func_def(FDEF_T, $2, $4, $7, ret);

           struct symlist* parent_symtab = current_symtab->next;
           if (parent_symtab != NULL) {
               insert_sym(parent_symtab, $2, ret, FUNCTION_SYM, $4, yylineno, line);
           }
           // ret_type = ret; // Questa variabile globale non sembra critica per la logica principale
           insert_sym(current_symtab, "$return", ret, F_RETURN, NULL, yylineno, line); // Simbolo per il tipo di ritorno
           scope_exit();
         }
     | FUNCTION ID '(' ')'
         { scope_enter(); param_list = NULL;} // current_symtab è ora lo scope della funzione
             chunk   END
         {
           enum LUA_TYPE ret = infer_func_return_type($6, current_symtab); // Passa current_symtab
           $$ = new_func_def(FDEF_T, $2, NULL, $6, ret);
           struct symlist* parent_symtab = current_symtab->next;
           if (parent_symtab != NULL) {
               insert_sym(parent_symtab, $2, ret, FUNCTION_SYM, NULL, yylineno, line);
           }
           // ret_type = ret;
           insert_sym(current_symtab, "$return", ret, F_RETURN, NULL, yylineno, line);
           scope_exit();
         }
     | name_or_ioread '=' FUNCTION  '(' param_list ')'
         { scope_enter(); param_list = $5; } // current_symtab è ora lo scope della funzione
             chunk   END
         {
           enum LUA_TYPE ret = infer_func_return_type($8, current_symtab); // Passa current_symtab
           char* func_name_str = NULL;
           if ($1->nodetype == VAR_T && $1->node.var) {
               func_name_str = $1->node.var->name; // Il nome a cui la funzione è assegnata
           }
           // Crea il nodo FDEF. Se func_name_str è NULL, è effettivamente anonima.
           // new_func_def dovrebbe fare strdup del nome se lo memorizza.
           struct AstNode* fdef_node = new_func_def(FDEF_T, func_name_str ? strdup(func_name_str) : NULL, $5, $8, ret);

           // Crea un nodo di assegnazione: name_or_ioread = fdef_node
           // $$ = new_expression(EXPR_T, ASS_T, $1, fdef_node);
           // La tua grammatica ha questo in func_definition, non assignment.
           // Quindi $$ deve essere un FDEF_T, ma l'assegnazione a name_or_ioread
           // deve essere gestita. Per ora, assumo che il parser si aspetti FDEF_T.
           $$ = fdef_node;


           // Aggiungi alla symbol table del parent scope
           if (func_name_str) {
               struct symlist* parent_symtab = current_symtab->next;
               if (parent_symtab != NULL) {
                   insert_sym(parent_symtab, func_name_str, ret, FUNCTION_SYM, $5, yylineno, line);
               }
           }
           // ret_type = ret;
           insert_sym(current_symtab, "$return", ret, F_RETURN, NULL, yylineno, line);
           scope_exit();
         }
     ;

param_list
    : param
    | param ',' param_list                                          { $$ = link_AstNode($1, $3); }
    ;

param
    : ID
        { $$ = new_variable(VAR_T, $1, NULL); }
    | STRING
        { $$ = new_value(VAL_T, STRING_T, $1); }
    | BOOL                                                           { $$ = new_value(VAL_T, eval_bool($1), $1); }
    | NIL                                                            { $$ = new_value(VAL_T, NIL_T, NULL); }
    | ID '=' number                                                  { $$ = new_declaration(DECL_T, new_variable(VAR_T, $1, NULL), $3); }
    | ID '=' STRING                                                  { $$ = new_declaration(DECL_T, new_variable(VAR_T, $1, NULL), new_value(VAL_T, STRING_T, $3)); }
    | ID '=' BOOL                                                    { $$ = new_declaration(DECL_T, new_variable(VAR_T, $1, NULL), new_value(VAL_T, eval_bool($3), $3)); }
    | ID '=' NIL                                                     { $$ = new_declaration(DECL_T, new_variable(VAR_T, $1, NULL), new_value(VAL_T, NIL_T, NULL)); }
    ;

table_list
    : table_field
    | table_field ',' table_list                                    { $$ = link_AstNode($1, $3); }
    ;

table_field
    : ID
        { $$ = new_table_field(TABLE_FIELD_T, new_variable(VAR_T, $1, NULL), NULL); }
    | ID '=' expr
        { $$ = new_table_field(TABLE_FIELD_T, new_variable(VAR_T, $1, NULL), $3); }
    | INT_NUM
        { $$ = new_table_field(TABLE_FIELD_T, NULL, new_value(VAL_T, INT_T, $1)); }
    | FLOAT_NUM
        { $$ = new_table_field(TABLE_FIELD_T, NULL, new_value(VAL_T, FLOAT_T, $1)); }
    | STRING
        { $$ = new_table_field(TABLE_FIELD_T, NULL, new_value(VAL_T, STRING_T, $1)); }
    | BOOL
        { $$ = new_table_field(TABLE_FIELD_T, NULL, new_value(VAL_T, eval_bool($1), $1)); }
    | '{' table_list '}'
        { $$ = new_table(TABLE_NODE_T, $2); }
    | /* empty */
        { $$ = new_table_field(TABLE_FIELD_T, NULL, NULL); }

 assignment
     : name_or_ioread '=' expr
         { $$ = new_expression(EXPR_T, ASS_T, $1, $3); fill_symtab(current_symtab, $$, eval_expr_type($3, current_symtab).type, VARIABLE); }
     ;

chunk
    : statement_list                                               { $$ = $1; }
    ;

statement_list
    : statement
    | statement statement_list                                       { $$ = link_AstNode($1, $2); }
    ;

statement
    : assignment
    | func_call
    | selection_statement
    | iteration_statement
    | return_statement
    ;

selection_statement
    : IF if_cond THEN
      { scope_enter(); }
      chunk
      { $<ast>$ = $5; }
      selection_ending
      {
        /* if else exists */
        if ($7)
          $$ = new_if(IF_T, $2, $<ast>6, $7);
        else // else end
          $$ = new_if(IF_T, $2, $<ast>6, NULL);
      }
    ;

selection_ending
    : END
      { scope_exit(); $$ = NULL;} // No else
    | ELSE
      { scope_exit(); scope_enter(); }
      chunk END
      { $$ = $3;  scope_exit(); } // Else
    ;

if_cond
    : expr
        { $$ = $1; }
    ;

iteration_statement
    : FOR ID '=' start_expr ',' end_expr step DO { scope_enter(); } chunk END               { $$ = new_for(FOR_T, $2, $4, $6, $7, $10); scope_exit(); }
    ;

start_expr
    : expr
        { $$ = $1; }
    | /* empty */
        {$$ = NULL; }
    ;

 end_expr
     : expr                                                          { $$ = $1; }
     | /* empty */                                                   { $$ = NULL; }
     ;

 step
     : ',' expr                                                    { eval_expr_type($2, current_symtab); $$ = $2; }
     | /* empty */                                                 { $$ = NULL; }
     ;

return_statement
    : RETURN optional_expr_list
      { $$ = new_return(RETURN_T, $2); }
    ;

optional_expr_list
    : /* empty */ { $$ = NULL; } %prec LOWEST
    | args        { $$ = $1; } %prec LOWEST
    ;

expr  // Definizione di base per le espressioni, costruisce sulla precedenza
    : primary_expr
    | expr '+' expr                                                 { $$ = new_expression(EXPR_T, ADD_T, $1, $3); }
    | expr '-' expr                                                 { $$ = new_expression(EXPR_T, SUB_T, $1, $3); }
    | expr '*' expr                                                 { $$ = new_expression(EXPR_T, MUL_T, $1, $3); }
    | expr '/' expr                                                 { $$ = new_expression(EXPR_T, DIV_T, $1, $3); check_division($3, current_symtab); }
    | NOT primary_expr                                              { $$ = new_expression(EXPR_T, NOT_T, NULL, $2); }
    | expr AND expr                                                 { $$ = new_expression(EXPR_T, AND_T, $1, $3); }
    | expr OR expr                                                  { $$ = new_expression(EXPR_T, OR_T, $1, $3); }
    | expr '>' expr                                                 { $$ = new_expression(EXPR_T, G_T, $1, $3); }
    | expr '<' expr                                                 { $$ = new_expression(EXPR_T, L_T, $1, $3); }
    | expr GE expr                                                  { $$ = new_expression(EXPR_T, GE_T, $1, $3); }
    | expr LE expr                                                  { $$ = new_expression(EXPR_T, LE_T, $1, $3); }
    | expr EQ expr                                                  { $$ = new_expression(EXPR_T, EQ_T, $1, $3); }
    | expr NE expr                                                  { $$ = new_expression(EXPR_T, NE_T, $1, $3); }
    | '-' primary_expr %prec UMINUS                                 { $$ = new_expression(EXPR_T, NEG_T, NULL, $2); }
    ;

primary_expr  // Espressioni "atomiche" o che non sono operatori binari/unari di livello superiore
    : name_or_ioread           { $$ = $1; }
    | number                   { $$ = $1; }
    | STRING                   { $$ = new_value(VAL_T, STRING_T, $1); }
    | NIL                      { $$ = new_value(VAL_T, NIL_T, NULL); }
    | BOOL                     { $$ = new_value(VAL_T, eval_bool($1), $1); }
    | '{' table_list '}'       { $$ = new_table(TABLE_NODE_T, $2); }
    | func_call                { $$ = $1; }
    | '(' expr ')'             { $$ = new_expression(EXPR_T, PAR_T, NULL, $2); }
    ;

name_or_ioread
    : ID { $$ = new_variable(VAR_T, $1, NULL); }
    | ID DOT ID { $$ = new_io_read_identifier_node($1, $3); } // Usa la funzione helper
    ;

number
    : INT_NUM
        { $$ = new_value(VAL_T, INT_T, $1); }
    | FLOAT_NUM
        { $$ = new_value(VAL_T, FLOAT_T, $1); }
    ;

func_call
    : name_or_ioread '(' args ')' { // name_or_ioread può essere 'ID' o 'io.read'
                                     $$ = new_func_call(FCALL_T, $1, $3);
                                     check_fcall($1, $3); // Chiamata alla funzione di controllo semantico

                                     // Aggiorna il tipo di ritorno in base alla definizione della funzione
                                     if ($1->nodetype == VAR_T) {
                                         struct symbol* func_sym = find_symtab(current_symtab, $1->node.var->name);
                                         if (func_sym) {
                                             $$->node.fcall->return_type = func_sym->type;
                                         }
                                     }
                                   }
    | name_or_ioread '(' ')'      {
                                     $$ = new_func_call(FCALL_T, $1, NULL);
                                     check_fcall($1, NULL);

                                     // Aggiorna il tipo di ritorno in base alla definizione della funzione
                                     if ($1->nodetype == VAR_T) {
                                         struct symbol* func_sym = find_symtab(current_symtab, $1->node.var->name);
                                         if (func_sym) {
                                             $$->node.fcall->return_type = func_sym->type;
                                         }
                                     }
                                   }
    ;

args
    : expr
    | expr ',' args                                                 { $$ = link_AstNode($1, $3); }
    ;

%%


int main(int argc, char **argv) {
    int file_count = 0;

    if(argc < 2) {
        fprintf(stderr, RED "fatal error:" RESET " no input file\n");
        exit(1);
    }else{
        for(int i = 1; i < argc; i++){
            if(strcmp(argv[i], "-s") == 0)
                print_symtab_flag = 1;
            else if(strcmp(argv[i], "-t") == 0)
                print_ast_flag = 1;
            else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
                print_usage();
                exit(0);
            }else {
                if(argv[i][0] == '-'){
                    fprintf(stderr, RED "error:" RESET " unrecognized command line option " BOLD "%s \n" RESET, argv[i]);
                    print_usage();
                    exit(1);
                }else{
                    if(file_count == 0){
                        filename = argv[i];
                        file_count++;
                    }else{
                        fprintf(stderr, RED "error:" RESET " only one input file accepted \n");
                        exit(1);
                    }
                }
            }
        }
    }

    if(file_count == 0){
        fprintf(stderr, RED "fatal error:" RESET " no input file\n");
        exit(1);
    }else{
        yyin = fopen(filename, "r");
    }

    if(!yyin) {
        fprintf(stderr, RED "error:" RESET " %s: ", filename);
        perror("");
        fprintf(stderr, RED "fatal error:" RESET" no input file\n");
        exit(1);
    }

    // inizializzazione variabili globali
    error_num = 0;
    current_scope_lvl = 0;
    current_symtab = NULL;

    if(yyparse() == 0) {

        if(print_ast_flag)
            print_ast(root);
        if(error_num == 0){

            translate(root);
        }
    }

    fclose(yyin);
}


/* Apre un nuovo scope. Se lo scope è quello di una funzione vi aggiunge
   eventuali parametri, presenti in param_list, e il tipo di ritorno della funzione
*/
void scope_enter() {
    current_symtab = create_symtab(current_scope_lvl, current_symtab);
    // La prima volta che entriamo in uno scope, salviamo il puntatore alla tabella globale
    if (root_symtab == NULL) {
        root_symtab = current_symtab;
    }
    current_scope_lvl++;

    if(param_list) {
        fill_symtab(current_symtab, param_list, -1, PARAMETER);
        param_list = NULL;
    }

    // We now handle function return types explicitly in the function definition rules
    // so we don't need to add them here automatically
    ret_type = -1; // Reset ret_type to default value
}

/* Chiude uno scope. Eventualmente effettua il print della Symbol Table.
   Verifica che le variabili dichiarate all'interno dello scope siano utilizzate
*/
void scope_exit() {
    if(print_symtab_flag)
        print_symtab(current_symtab);
    //check_usage(current_symtab);

    current_symtab = current_symtab->next;
    current_scope_lvl--;
}



/* Usata per inserire uno o più simboli all'interno della Symbol Table.
   Usata per 1 - Dichiarazione di variabili con assegnazione
             2 - Parametri di una funzione
*/
void fill_symtab(struct symlist * syml, struct AstNode *n, enum LUA_TYPE type, enum sym_type sym_type) {
    if (!n) return;

    switch (n->nodetype) {
        case VAR_T:
            insert_sym(syml, n->node.var->name, type, sym_type, NULL, yylineno, line);
            break;
        case EXPR_T:
            if (n->node.expr->expr_type == ASS_T && n->node.expr->l->nodetype == VAR_T) {
                insert_sym(syml, n->node.expr->l->node.var->name, type, sym_type, NULL, yylineno, line);
            }
            break;
        case DECL_T:
            fill_symtab(syml, n->node.decl->var, type, sym_type);
            break;
    }
}

/*  Funzione richiamata in caso si usi il flag --help o -h.
    Mostra a schermo l'uso del transpilatore
*/
void print_usage(){
    printf("Usage: ./compiler [options] file  \n");
    printf("options: \n");
    printf(" --help \t Display this information. \n");
    printf(" -h \t\t Display this information. \n");
    printf(" -s \t\t Print Symbol Table. \n");
    printf(" -t \t\t Print Abstract Syntax Tree. \n");
}
