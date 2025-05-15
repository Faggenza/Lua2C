%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "semantic.h"
#include "symtab.h"
#include "global.h"
#include "pretty.h"

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

enum LUA_TYPE type;

void print_ast(struct AstNode *root);
void translate(struct AstNode *root);
void check_var_reference(struct AstNode *var);

%}

//%error-verbose
%define parse.error verbose

%expect 4 /* conflitto shift/reduce (- per ruturn) , bison lo risolve scegliendo shift */

%union {
    char* s;
    struct AstNode *ast;
    int t;
}

%token  INT_NUM FLOAT_NUM
%token  IF ELSE THEN FOR DO END RETURN
%token  FUNCTION
%token  <s> PRINT READ 
%token  <s> STRING BOOL NIL
%left   AND OR
%left   '>' '<' GE LE EQ NE
%left   '+' '-'        // Operatori binari con stessa precedenza
%left   '*' '/'        // Operatori moltiplicativi con precedenza maggiore
%right  NOT
%right  '='
%precedence LOWEST  // Precedenza bassa per return senza expr
%precedence UMINUS          // Precedenza alta per il meno unario
%nonassoc EXPR_START

// Indica i token che possono iniziare un'espressione
%nonassoc <s> INT_NUM FLOAT_NUM
%nonassoc <s> ID
%nonassoc '('

%type <ast> number global_statement_list global_statement expr  assignment
%type <ast> return_statement expr_statement func_call args if_cond //print_statement 
%type <ast> statement_list statement
%type <ast> name
%type <ast> func_definition param_list param table_field table_list chunk
%type <ast> iteration_statement start end step selection_statement
//%type <t> type

%%

program
    : global_statement_list                                         { root = $1; }         
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
    | expr_statement
    | return_statement
    ;

func_definition
    : FUNCTION ID '(' param_list ')'                                
        { param_list = $4; /* No type checking needed */ } 
            chunk   END                                      
        { $$ = new_func_def(FDEF_T, $2, $4, $7); }
    | FUNCTION ID '(' ')'                                           
        { /* No return type needed */ } 
            chunk   END                                      
        { $$ = new_func_def(FDEF_T, $2, NULL, $6); }
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
    //| number                                                         { $$ = new_value(VAL_T, INT_NUM, $1); }
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
        { $$ = new_table_field(TABLE_FIELD_T, new_value(VAL_T, INT_NUM, $1), NULL); }
    | FLOAT_NUM                                                     
        { $$ = new_table_field(TABLE_FIELD_T, new_value(VAL_T, FLOAT_NUM, $1), NULL); }
    | '{' table_list '}'
        { $$ = new_table(TABLE_T, $2); }
    | /* empty */                                                 
        { $$ = new_table_field(TABLE_FIELD_T, NULL, NULL); }
    
assignment
    : name '=' expr                                                 
        { $$ = new_expression(EXPR_T, ASS_T, $1, $3); }
    ;

chunk
    : statement_list                                               { $$ = $1; }
    ;

statement_list
    : statement   
    | statement_list statement                                      { $$ = link_AstNode($1, $2); }
    ;

statement
    : assignment
    | expr_statement
    | selection_statement
    | iteration_statement
    | return_statement
//    | print_statement                                             { fmt_flag = 1; }
//    | read_statement                                               { fmt_flag = 1; }
    //| assignment_statement
    ;


expr_statement
    : expr %prec LOWEST                                                        
        { $$ = check_expr_statement($1); }
    ;

selection_statement
    : IF  if_cond THEN chunk END                              { $$ = new_if(IF_T, $2, $4, NULL); }
    | IF  if_cond THEN chunk ELSE chunk END                   { $$ = new_if(IF_T, $2, $4, $6); }
    ;

if_cond
    : expr                                                          
        { $$ = $1; }
    ;

iteration_statement
    : FOR ID '=' start ',' end step DO chunk END               { $$ = new_for(FOR_T, $2, $4, $6, $7, $9); scope_exit(); }
    ;

start
    : expr
        { scope_enter(); $$ = new_declaration(DECL_T, $1, NULL); fill_symtab(current_symtab, $$, 0, VARIABLE); }
    | /* empty */
        { scope_enter(); $$ = NULL; }
    ;

end
    : expr                                                          { check_cond(eval_expr_type($1).type); $$ = $1; }
    | /* empty */                                                   { $$ = NULL; }
    ;

step
    : ',' expr                                                    { eval_expr_type($2); $$ = $2; }
    | /* empty */                                                   { $$ = NULL; }
    ;

return_statement
    : RETURN %prec LOWEST                                 
        { $$ = new_return(RETURN_T, NULL); }
    | RETURN expr %prec LOWEST                                 
        { $$ = new_return(RETURN_T, $2); }
    ;

//print_statement
//    : PRINT '(' args ')'                                   
//        { $$ = new_func_call(FCALL_T, new_variable(VAR_T, $1, NULL), $3); }
//    | PRINT '(' STRING ',' args ')'                          
//        { $$ = new_func_call(FCALL_T, new_variable(VAR_T, $1, NULL), 
//            link_AstNode(new_value(VAL_T, STRING_T, $3), $5)); 
//        check_format_string(new_value(VAL_T, STRING_T, $3), $5, PRINT_T); }
//    | PRINT '(' ')'                                                 
//        { $$ = new_error(ERROR_NODE_T); yyerror("too few arguments to function" BOLD " print" RESET); }
//    ;

//da controllare read Lua
//read_statement
//    : READ '(' format_string ',' read_name_list ')'                { check_format_string($3, $5, READ_T); $$ = new_func_call(FCALL_T, $1, link_AstNode($3, $5)); }
//    | READ '(' format_string ')'                                   { check_format_string($3, NULL, READ_T); $$ = new_func_call(FCALL_T, $1, $3); }
//    | READ '(' ')'                                                 { $$ = new_error(ERROR_NODE_T); yyerror("too few arguments to function" BOLD " read" RESET); }
//    ;
//
//read_name_list
//    : '&' name                                                       { check_var_reference($2); by_reference($2); $$ = $2; }
//    | name                                                           { check_var_reference($1); $$ = $1; }
//    | '&' name ',' read_var_list                                    { check_var_reference($2); by_reference($2); $$ = link_AstNode($2, $4); }
//    | name ','   read_var_list                                      { check_var_reference($1); link_AstNode($1, $3); }
//    ;

expr
    : name                                           
    | number
    | func_call
    | STRING                                                        { $$ = new_value(VAL_T, STRING_T, $1); }
    | NIL                                                           { $$ = new_value(VAL_T, NIL_T, NULL);  }
    | BOOL                                                          { $$ = new_value(VAL_T, eval_bool($1), $1); }
    | '{' table_list '}'                                            { $$ = new_table(TABLE_T, $2); }
    | expr '+' expr                                                 { $$ = new_expression(EXPR_T, ADD_T, $1, $3); }
    | expr '-' expr                                                 { $$ = new_expression(EXPR_T, SUB_T, $1, $3); }
    | expr '*' expr                                                 { $$ = new_expression(EXPR_T, MUL_T, $1, $3); }
    | expr '/' expr                                                 { $$ = new_expression(EXPR_T, DIV_T, $1, $3); check_division($3); }
    | NOT expr                                                      { $$ = new_expression(EXPR_T, NOT_T, NULL, $2); }
    | expr AND expr                                                 { $$ = new_expression(EXPR_T, AND_T, $1, $3); }
    | expr OR expr                                                  { $$ = new_expression(EXPR_T, OR_T, $1, $3); }
    | expr '>' expr                                                 { $$ = new_expression(EXPR_T, G_T, $1, $3); }
    | expr '<' expr                                                 { $$ = new_expression(EXPR_T, L_T, $1, $3); }
    | expr GE expr                                                  { $$ = new_expression(EXPR_T, GE_T, $1, $3); }
    | expr LE expr                                                  { $$ = new_expression(EXPR_T, LE_T, $1, $3); }
    | expr EQ expr                                                  { $$ = new_expression(EXPR_T, EQ_T, $1, $3); }
    | expr NE expr                                                  { $$ = new_expression(EXPR_T, NE_T, $1, $3); }
    | '(' expr ')'                                                  { $$ = new_expression(EXPR_T, PAR_T, NULL, $2); }
    | '-' expr %prec UMINUS                                         { $$ = new_expression(EXPR_T, NEG_T, NULL, $2); }
    ;

name
    : ID                                                          
        { $$ = new_variable(VAR_T, $1, NULL); }
    ;

number
    : INT_NUM                                                       
        { $$ = new_value(VAL_T, INT_NUM, $1); }
    | FLOAT_NUM                                                     
        { $$ = new_value(VAL_T, FLOAT_NUM, $1); }
    ;

func_call
    : ID '(' args ')'                                               { $$ = new_func_call(FCALL_T, new_variable(VAR_T, $1, NULL), $3); check_fcall($1, $3); }
    | ID '(' ')'                                                    { $$ = new_func_call(FCALL_T, new_variable(VAR_T, $1, NULL), NULL); check_fcall($1, NULL); }
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

    // inizializzazione nameiabili globali
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
    current_scope_lvl++;

    if(param_list) { 
        fill_symtab(current_symtab, param_list, -1, PARAMETER); 
        param_list = NULL;
    }

    if(ret_type != -1) {
        insert_sym(current_symtab, "return", ret_type, F_RETURN, NULL, yylineno, line);
        ret_type = -1;
    }
}

/* Chiude uno scope. Eventualmente effettua il print della Symbol Table.
   Verifica che le nameiabili dichiarate all'interno dello scope siano utilizzate
*/
void scope_exit() {
    if(print_symtab_flag)
        print_symtab(current_symtab);
    check_usage(current_symtab); 
    current_symtab = delete_symtab(current_symtab); 
    current_scope_lvl--;
}



/* Usata per inserire uno o più simboli all'interno della Symbol Table. 
   Usata per 1 - Dichiarazione di variabili
             2 - Dichiarazione di variabili con assegnazione
             3 - Parametri di una funzione
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
        // Altri casi...
    }
}

/*  Funzione richiamata in caso si usi il flag --help o -h.
    Mostra a schermo l'uso del compilatore 
*/
void print_usage(){
    printf("Usage: ./compiler [options] file  \n");
    printf("options: \n");
    printf(" --help \t Display this information. \n");
    printf(" -h \t\t Display this information. \n");
    printf(" -s \t\t Print Symbol Table. \n");
    printf(" -t \t\t Print Abstract Syntax Tree. \n");
}
