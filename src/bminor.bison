%{

#include "parser_diag.h"
#include <stdio.h>
#include <stdlib.h>

static int err_count = 0;

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
}
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
program   : decl_list
          ;

decl_list : %empty                      //{ $$ = NULL; }
          | decl_list decl
          ;

decl      : TOKEN_IDENTIFIER ':' var_type ';'
          | TOKEN_IDENTIFIER ':' var_type '=' init ';'
          | TOKEN_IDENTIFIER ':' func_type ';'
          | TOKEN_IDENTIFIER ':' func_type '=' block
          ;

/* ------- INITIALIZERS ------- */
init      : expr
          | '{' init_list '}'
          ;

init_list : init
          | init_list ',' init
          ;

/* ------- BLOCKS ------- */
block           : '{' block_item_list '}'
                ;

block_item_list : %empty                        //{ $$ = NULL; }
                | block_item_list block_item
                ;

block_item      : decl
                | stmt
                ;

/* ------- TYPES ------- */
func_type       : TOKEN_FUNCTION return_type '(' opt_param_list ')'
                ;

return_type     : atomic_type
                | TOKEN_VOID
                | TOKEN_AUTO
                ;

opt_param_list  : %empty                      //{ $$ = NULL; }
                | param_list
                ;

param_list      : param
                | param_list ',' param
                ;

param           : TOKEN_IDENTIFIER ':' param_type
                ;

param_type      : atomic_type
                | TOKEN_ARRAY '[' ']' param_type    /* No lengths on array params */
                | TOKEN_CARRAY '[' ']' param_type
                ;

var_type        : TOKEN_AUTO
                | simple_type
                ;

simple_type     : atomic_type
                | TOKEN_ARRAY '[' expr ']' simple_type
                | TOKEN_CARRAY '[' expr ']' simple_type
                ;

atomic_type     : TOKEN_BOOLEAN
                | TOKEN_CHAR
                | TOKEN_DOUBLE
                | TOKEN_INTEGER
                | TOKEN_STRING
                ;

/* ------- STATEMENTS ------- */
stmt            : matched_stmt
                | unmatched_stmt    /* Dangling else resolution */
                ;

matched_stmt    : simple_stmt
                | TOKEN_IF '(' expr ')' matched_stmt TOKEN_ELSE matched_stmt
                | TOKEN_FOR '(' opt_expr ';' opt_expr ';' opt_expr ')' matched_stmt
                ;

unmatched_stmt  : TOKEN_IF '(' expr ')' stmt
                | TOKEN_IF '(' expr ')' matched_stmt TOKEN_ELSE unmatched_stmt
                | TOKEN_FOR '(' opt_expr ';' opt_expr ';' opt_expr ')' unmatched_stmt
                ;


simple_stmt     : expr ';'
                | TOKEN_RETURN ';'
                | TOKEN_RETURN expr ';'
                | TOKEN_PRINT ';'
                | TOKEN_PRINT expr_list ';'
                | block
                ;

/* ------- EXPRESSIONS ------- */
opt_expr_list : %empty                      //{ $$ = NULL; }
              | expr_list
              ;

expr_list     : expr
              | expr_list ',' expr
              ;

opt_expr      : %empty                      //{ $$ = NULL; }
              | expr
              ;

expr          : assign_expr
              ;

assign_expr   : or_expr '=' assign_expr        /* right-associative */
              | or_expr
              ;

or_expr       : or_expr TOKEN_OR and_expr      /* left-associative */
              | and_expr
              ;

and_expr      : and_expr TOKEN_AND cmp_expr
              | cmp_expr
              ;

cmp_expr      : cmp_expr '<' add_expr
              | cmp_expr '>' add_expr
              | cmp_expr TOKEN_LE add_expr
              | cmp_expr TOKEN_GE add_expr
              | cmp_expr TOKEN_EQ add_expr
              | cmp_expr TOKEN_NE add_expr
              | add_expr
              ;

add_expr      : add_expr '+' mul_expr
              | add_expr '-' mul_expr
              | mul_expr
              ;

mul_expr      : mul_expr '*' unary_expr
              | mul_expr '/' unary_expr
              | mul_expr '%' unary_expr
              | unary_expr
              ;

unary_expr    : '-' unary_expr
              | '!' unary_expr
              | len_expr
              ;

len_expr      : '#' len_expr
              | pow_expr
              ;

pow_expr      : postfix_expr '^' unary_expr    /* -x^2 -> -(x^2) */
              | postfix_expr
              ;

postfix_expr  : postfix_expr TOKEN_INC
              | postfix_expr TOKEN_DEC
              | postfix_expr '[' expr ']'
              | postfix_expr '(' opt_expr_list ')'
              | primary_expr
              ;

primary_expr  : TOKEN_IDENTIFIER
              | TOKEN_CHAR_LITERAL
              | TOKEN_DOUBLE_LITERAL
              | TOKEN_INTEGER_LITERAL
              | TOKEN_STRING_LITERAL
              | TOKEN_FALSE
              | TOKEN_TRUE
              | '(' expr ')'
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
