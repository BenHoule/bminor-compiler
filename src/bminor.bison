%{

#include "bminor_parser.h"
#include "decl.h"
#include "stmt.h"
#include "expr.h"
#include "type.h"
#include "param_list.h"

#include <stdio.h>
#include <stdlib.h>

static unsigned int err_count = 0;
struct decl *parser_result = nullptr;

int yylex(void);
int yyerror(const char *msg);

%}

/* Set Bison parser options */
%code requires {
#include <stdint.h>
}

%locations
%define parse.error detailed
%define parse.lac full
%destructor { free($$); } <sval>

/* Union of all possible semantic types */
%union {
  int64_t ival;
  double dval;
  char cval;
  char *sval;
  struct decl *decl;
  struct stmt *stmt;
  struct expr *expr;
  struct type *type;
  struct param_list *params;
}

/* Associate non-terminals/tokens with union fields */
%type <decl> program decl_list decl
%type <stmt> block block_item_list block_item
%type <stmt> stmt matched_stmt unmatched_stmt simple_stmt
%type <expr> init init_list expr expr_list opt_expr opt_expr_list
%type <expr> or_expr and_expr cmp_expr add_expr mul_expr unary_expr len_expr
%type <expr> pow_expr postfix_expr primary_expr
%type <type> var_type simple_type primitive_type func_type return_type param_type
%type <params> param opt_param_list param_list

%token <sval> TOKEN_IDENTIFIER "identifier"

/* LITERAL TOKENS */
%token <cval> TOKEN_CHAR_LITERAL "char_literal"
%token <dval> TOKEN_DOUBLE_LITERAL "double_literal"
%token <ival> TOKEN_INTEGER_LITERAL "integer_literal"
%token <sval> TOKEN_STRING_LITERAL "string_literal"

/* PRIMITIVE TYPE TOKENS */
%token TOKEN_BOOLEAN "boolean"
%token TOKEN_CHAR "char"
%token TOKEN_DOUBLE "double"
%token TOKEN_FLOAT "float"  /* Reserved for expansion(?) */
%token TOKEN_INTEGER "integer"
%token TOKEN_STRING "string"

/* NON-PRIMITIVE TYPE TOKENS */
%token TOKEN_ARRAY "array"
%token TOKEN_AUTO "auto"
%token TOKEN_CARRAY  "carray"
%token TOKEN_FUNCTION "function"
%token TOKEN_VOID "void"

/* CONTROL TOKENS */
%token TOKEN_ELSE "else"
%token TOKEN_FOR "for"
%token TOKEN_IF "if"
%token TOKEN_RETURN "return"
%token TOKEN_WHILE "while"  /* Reserved for expansion */
%token TOKEN_FALSE "false"
%token TOKEN_TRUE "true"

/* OPERATOR TOKENS (ops that use 2 chars) */
%token TOKEN_INC "++"
%token TOKEN_DEC "--"
%token TOKEN_AND "&&"
%token TOKEN_OR "||"
%token TOKEN_EQ "=="
%token TOKEN_GE ">="
%token TOKEN_LE "<="
%token TOKEN_NE "!="

/* PRINT (is a statement, not a function call) */
%token TOKEN_PRINT "print"

%%
program   : decl_list       { parser_result = $1; }
          ;

decl_list : decl decl_list  { $1->next = $2; $$ = $1; }
          | %empty          { $$ = nullptr; }
          ;

decl      : TOKEN_IDENTIFIER ':' var_type ';'             { $$ = decl_create($1, $3, nullptr, nullptr, nullptr); }
          | TOKEN_IDENTIFIER ':' var_type '=' init ';'    { $$ = decl_create($1, $3, $5, nullptr, nullptr); }
          | TOKEN_IDENTIFIER ':' func_type ';'            { $$ = decl_create($1, $3, nullptr, nullptr, nullptr); }
          | TOKEN_IDENTIFIER ':' func_type '=' block      { $$ = decl_create($1, $3, nullptr, $5, nullptr); }
          ;

/* ------- INITIALIZERS ------- */
init      : expr
          | '{' init_list '}'                   { $$ = expr_create(EXPR_INIT, $2, nullptr); }
          ;

init_list : init                                { $$ = expr_create(EXPR_ARG, $1, nullptr); }
          | init ',' init_list                  { $$ = expr_create(EXPR_ARG, $1, $3); }
          ;

/* ------- BLOCKS ------- */
block           : '{' block_item_list '}'       { $$ = stmt_create_block($2); }
                ;

block_item_list : block_item block_item_list    { $1->next = $2; $$ = $1; }
                | %empty                        { $$ = nullptr; }
                ;

block_item      : decl                          { $$ = stmt_create_decl($1); }
                | stmt                          { $$ = $1; }
                ;

/* ------- TYPES ------- */
func_type       : TOKEN_FUNCTION return_type '(' opt_param_list ')'     { $$ = type_create_function($2, $4); }
                ;

return_type     : primitive_type
                | TOKEN_VOID          { $$ = type_create_void(); }
                | TOKEN_AUTO          { $$ = type_create_auto(); }
                ;

opt_param_list  : param_list
                | %empty              { $$ = nullptr; }
                ;

param_list      : param
                | param ',' param_list         { $1->next = $3; $$ = $1; }
                ;

param           : TOKEN_IDENTIFIER ':' param_type   { $$ = param_list_create($1, $3, nullptr); }
                ;

param_type      : primitive_type         /* No lengths on array params */
                | TOKEN_ARRAY '[' ']' param_type    { $$ = type_create_array($4, nullptr); }
                | TOKEN_CARRAY '[' ']' param_type   { $$ = type_create_carray($4, nullptr); }
                ;

var_type        : simple_type
                | TOKEN_AUTO        { $$ = type_create_auto(); }
                ;

simple_type     : primitive_type
                | TOKEN_ARRAY '[' expr ']' simple_type     { $$ = type_create_array($5, $3); }
                | TOKEN_CARRAY '[' expr ']' simple_type    { $$ = type_create_carray($5, $3); }
                ;

primitive_type  : TOKEN_BOOLEAN       { $$ = type_create_boolean(); }
                | TOKEN_CHAR          { $$ = type_create_char(); }
                | TOKEN_DOUBLE        { $$ = type_create_double(); }
                | TOKEN_INTEGER       { $$ = type_create_integer(); }
                | TOKEN_STRING        { $$ = type_create_string(); }
                ;

/* ------- STATEMENTS ------- */
stmt            : matched_stmt
                | unmatched_stmt        /* Dangling else resolution */
                ;

matched_stmt    : simple_stmt
                | TOKEN_IF '(' expr ')' matched_stmt TOKEN_ELSE matched_stmt           { $$ = stmt_create_if_else($3, $5, $7); }
                | TOKEN_FOR '(' opt_expr ';' opt_expr ';' opt_expr ')' matched_stmt    { $$ = stmt_create_for($3, $5, $7, $9); }
                ;

unmatched_stmt  : TOKEN_IF '(' expr ')' stmt                                             { $$ = stmt_create_if_else($3, $5, nullptr); }
                | TOKEN_IF '(' expr ')' matched_stmt TOKEN_ELSE unmatched_stmt           { $$ = stmt_create_if_else($3, $5, $7); }
                | TOKEN_FOR '(' opt_expr ';' opt_expr ';' opt_expr ')' unmatched_stmt    { $$ = stmt_create_for($3, $5, $7, $9); }
                ;

simple_stmt     : block
                | expr ';'                      { $$ = stmt_create_expr($1); }
                | TOKEN_RETURN ';'              { $$ = stmt_create_return(nullptr); }
                | TOKEN_RETURN expr ';'         { $$ = stmt_create_return($2); }
                | TOKEN_PRINT ';'               { $$ = stmt_create_print(nullptr); }
                | TOKEN_PRINT expr_list ';'     { $$ = stmt_create_print($2); }
                ;

/* ------- EXPRESSIONS ------- */
opt_expr_list : expr_list
              | %empty                      { $$ = nullptr; }
              ;

expr_list     : expr                        { $$ = expr_create(EXPR_ARG, $1, nullptr); }
              | expr ',' expr_list          { $$ = expr_create(EXPR_ARG, $1, $3); }
              ;

opt_expr      : expr
              | %empty                      { $$ = nullptr; }
              ;

expr          : or_expr
              | or_expr '=' expr                { $$ = expr_create(EXPR_ASGN, $1, $3); }
              ; /* right-associative */

or_expr       : and_expr
              | or_expr TOKEN_OR and_expr       { $$ = expr_create(EXPR_OR, $1, $3); }
              ; /* left-associative */

and_expr      : cmp_expr
              | and_expr TOKEN_AND cmp_expr     { $$ = expr_create(EXPR_AND, $1, $3); }
              ;

cmp_expr      : add_expr
              | cmp_expr '<' add_expr           { $$ = expr_create(EXPR_LT, $1, $3); }
              | cmp_expr '>' add_expr           { $$ = expr_create(EXPR_GT, $1, $3); }
              | cmp_expr TOKEN_LE add_expr      { $$ = expr_create(EXPR_LE, $1, $3); }
              | cmp_expr TOKEN_GE add_expr      { $$ = expr_create(EXPR_GE, $1, $3); }
              | cmp_expr TOKEN_EQ add_expr      { $$ = expr_create(EXPR_EQ, $1, $3); }
              | cmp_expr TOKEN_NE add_expr      { $$ = expr_create(EXPR_NE, $1, $3); }
              ;

add_expr      : mul_expr
              | add_expr '+' mul_expr           { $$ = expr_create(EXPR_ADD, $1, $3); }
              | add_expr '-' mul_expr           { $$ = expr_create(EXPR_SUB, $1, $3); }
              ;

mul_expr      : unary_expr
              | mul_expr '*' unary_expr         { $$ = expr_create(EXPR_MUL, $1, $3); }
              | mul_expr '/' unary_expr         { $$ = expr_create(EXPR_DIV, $1, $3); }
              | mul_expr '%' unary_expr         { $$ = expr_create(EXPR_MOD, $1, $3); }
              ;

unary_expr    : len_expr
              | '-' unary_expr                  { $$ = expr_create(EXPR_NEG, $2, nullptr); }
              | '!' unary_expr                  { $$ = expr_create(EXPR_NOT, $2, nullptr); }
              ;

len_expr      : pow_expr
              | '#' len_expr                    { $$ = expr_create(EXPR_LEN, $2, nullptr); }
              ;

pow_expr      : postfix_expr
              | postfix_expr '^' unary_expr    { $$ = expr_create(EXPR_POW, $1, $3); }  /* -x^2 -> -(x^2) */
              ;

postfix_expr  : primary_expr
              | postfix_expr TOKEN_INC                { $$ = expr_create(EXPR_INC,  $1, nullptr); }
              | postfix_expr TOKEN_DEC                { $$ = expr_create(EXPR_DEC,  $1, nullptr); }
              | postfix_expr '[' expr ']'             { $$ = expr_create(EXPR_IDX,  $1, $3); }
              | postfix_expr '(' opt_expr_list ')'    { $$ = expr_create(EXPR_CALL, $1, $3); }
              ;

primary_expr  : '(' expr ')'            { $$ = $2; }
              | TOKEN_IDENTIFIER        { $$ = expr_create_name($1); free($1); }
              | TOKEN_CHAR_LITERAL      { $$ = expr_create_char_literal($1); }
              | TOKEN_DOUBLE_LITERAL    { $$ = expr_create_double_literal($1); }
              | TOKEN_INTEGER_LITERAL   { $$ = expr_create_integer_literal($1); }
              | TOKEN_STRING_LITERAL    { $$ = expr_create_string_literal($1); free($1); }
              | TOKEN_FALSE             { $$ = expr_create_false(); }
              | TOKEN_TRUE              { $$ = expr_create_true(); }
              ;

%%

/* External access to parser err_count variable */
unsigned int parser_error_count(void) {
  return err_count;
}

/* Bison error handling */
int yyerror(const char *msg) {
  err_count++;
  fprintf(stderr, "parse error: [line %d, col %d] %s\n",
          yylloc.first_line, yylloc.first_column, msg);
  return 0;
}

/* Character tokens print as themselves; some aren't in the grammar yet,
  * so YYTRANSLATE would map them to the "invalid token" symbol.
  * Remove this check once all character tokens are included in the grammar. */
const char *token_name(int token) {
  if (0 < token && token < 256) {
    static char char_buf[2];
    char_buf[0] = (char)token;
    char_buf[1] = '\0';
    return char_buf;
  }
  return yysymbol_name(YYTRANSLATE(token));
}
