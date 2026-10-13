%{

#include "bminor_parser.h"
#include "decl.h"
#include "stmt.h"
#include "expr.h"
#include "type.h"
#include "param_list.h"

#include <stdio.h>
#include <stdlib.h>

static int err_count = 0;
struct decl *parser_result = NULL;

int yylex(void);
int yyerror(const char *msg);

%}

%locations
%define parse.error detailed
%define parse.lac full
%destructor { free($$); } <sval>

/* Union of all possible semantic types */
%union {
  long long ival;
  double dval;
  char cval;
  char *sval;
  struct decl *decl;
  struct stmt *stmt;
  struct expr *expr;
  struct type *type;
  struct param_list *params;
}
%type <decl> program decl_list decl
%type <stmt> block block_item_list block_item
%type <stmt> stmt matched_stmt unmatched_stmt simple_stmt
%type <expr> init init_list expr expr_list opt_expr opt_expr_list
%type <expr> or_expr and_expr cmp_expr add_expr mul_expr unary_expr len_expr
%type <expr> pow_expr postfix_expr primary_expr
%type <type> var_type simple_type atomic_type func_type return_type param_type
%type <params> param opt_param_list param_list
/* Associate non-terminals/tokens with union fields */

%token <sval> TOKEN_IDENTIFIER "identifier"

/* LITERAL TOKENS */
%token <cval> TOKEN_CHAR_LITERAL "char_literal"
%token <dval> TOKEN_DOUBLE_LITERAL "double_literal"
%token <ival> TOKEN_INTEGER_LITERAL "integer_literal"
%token <sval> TOKEN_STRING_LITERAL "string_literal"

/* ATOMIC TYPE TOKENS */
%token TOKEN_BOOLEAN "boolean"
%token TOKEN_CHAR "char"
%token TOKEN_DOUBLE "double"
%token TOKEN_FLOAT "float"  /* Reserved for expansion(?) */
%token TOKEN_INTEGER "integer"
%token TOKEN_STRING "string"

/* NON-ATOMIC TYPE TOKENS */
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
program     : expr ';'                  { parser_result = $1; }
            ;

opt_list    : %empty                     { $$ = NULL; }
            | expr_list                  {  }
            ;

expr        : assign_expr                {  }
            ;

expr_list   : assign_expr ',' expr_list         {  }
            | assign_expr                       {  }
            ;

assign_expr : or_expr '=' assign_expr        /* right-associative */
            | or_expr
            ;

or_expr     : or_expr TOKEN_OR and_expr      /* left-associative */
            | and_expr
            ;

and_expr    : and_expr TOKEN_AND cmp_expr
            | cmp_expr
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
              | '-' unary_expr                  { $$ = expr_create(EXPR_NEG, $2, NULL); }
              | '!' unary_expr                  { $$ = expr_create(EXPR_NOT, $2, NULL); }
              ;

len_expr      : pow_expr
              | '#' len_expr                    { $$ = expr_create(EXPR_LEN, $2, NULL); }
              ;

pow_expr      : postfix_expr
              | postfix_expr '^' unary_expr    { $$ = expr_create(EXPR_POW, $1, $3); }  /* -x^2 -> -(x^2) */
              ;

postfix_expr  : primary_expr
              | postfix_expr TOKEN_INC                { $$ = expr_create(EXPR_INC,  $1, NULL); }
              | postfix_expr TOKEN_DEC                { $$ = expr_create(EXPR_DEC,  $1, NULL); }
              | postfix_expr '[' expr ']'             { $$ = expr_create(EXPR_IDX,  $1, $3); }
              | postfix_expr '(' opt_expr_list ')'    { $$ = expr_create(EXPR_CALL, $1, $3); }
              ;

primary_expr  : '(' expr ')'            { $$ = $2; }
              | TOKEN_IDENTIFIER        { $$ = expr_create_name($1); }
              | TOKEN_CHAR_LITERAL      { $$ = expr_create_char_literal($1); }
              | TOKEN_DOUBLE_LITERAL    { $$ = expr_create_double_literal($1); }
              | TOKEN_INTEGER_LITERAL   { $$ = expr_create_integer_literal($1); }
              | TOKEN_STRING_LITERAL    { $$ = expr_create_string_literal($1); }
              | TOKEN_FALSE             { $$ = expr_create_false(); }
              | TOKEN_TRUE              { $$ = expr_create_true(); }
              ;

%%
unsigned int parser_error_count(void) {
  return err_count;
}

int yyerror(const char *msg) {
  err_count++;
  fprintf(stderr, "line %d, col %d: %s\n",
          yylloc.first_line, yylloc.first_column, msg);
  return 0;
}

const char *token_name(int token) {
  /* Character tokens print as themselves; some aren't in the grammar yet,
   * so YYTRANSLATE would map them to the "invalid token" symbol.
   * Remove this check once all character tokens are included in the grammar. */
  if (0 < token && token < 256) {
    static char char_buf[2];
    char_buf[0] = (char)token;
    char_buf[1] = '\0';
    return char_buf;
  }
  return yysymbol_name(YYTRANSLATE(token));
}
