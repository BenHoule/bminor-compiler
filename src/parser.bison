%{

#include "parser_diag.h"
#include <errno.h>
#include <stdio.h>

static int err_count = 0;
extern int yylineno;  /* defined in scanner.flex (`%option yylineno`) */

int yylex(void);
int yyerror(const char *msg);

struct node {
  int data;
  struct node* next;
};

%}

%destructor { free($$); } <sval>


/* Union of all possible semantic types */
%union {
  long long ival;
  double dval;
  char cval;
  char *sval;
  struct node* list_ptr;
}
/* Associate non-terminals/tokens with union fields */
%type <list_ptr> opt_list
%type <list_ptr> expr_list

%token <sval> TOKEN_IDENTIFIER

/* ATOMIC TYPE TOKENS */
%token TOKEN_BOOLEAN
%token TOKEN_CHAR
%token TOKEN_DOUBLE
%token TOKEN_FLOAT  /* Reserved for expansion(?) */
%token TOKEN_INTEGER
%token TOKEN_STRING

/* NON-ATOMIC TYPE TOKENS */
%token TOKEN_ARRAY
%token TOKEN_AUTO
%token TOKEN_CARRAY
%token TOKEN_FUNCTION
%token TOKEN_VOID

/* LITERAL TOKENS */
%token <cval> TOKEN_CHAR_LITERAL
%token <dval> TOKEN_DOUBLE_LITERAL
%token <ival> TOKEN_INTEGER_LITERAL
%token <sval> TOKEN_STRING_LITERAL
%token TOKEN_FALSE
%token TOKEN_TRUE

/* CONTROL TOKENS */
%token TOKEN_ELSE
%token TOKEN_FOR
%token TOKEN_IF
%token TOKEN_RETURN
%token TOKEN_WHILE  /* Reserved for expansion */

/* OPERATOR TOKENS (ops that use 2 chars) */
%token TOKEN_INC
%token TOKEN_DEC
%token TOKEN_AND
%token TOKEN_OR
%token TOKEN_EQ
%token TOKEN_GE
%token TOKEN_LE
%token TOKEN_NE

/* PRINT (is a statement, not a function call) */
%token TOKEN_PRINT

%%
program     : expr ';'
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

cmp_expr    : cmp_expr '<' add_expr
            | cmp_expr '>' add_expr
            | cmp_expr TOKEN_LE add_expr
            | cmp_expr TOKEN_GE add_expr
            | cmp_expr TOKEN_EQ add_expr
            | cmp_expr TOKEN_NE add_expr
            | add_expr
            ;

add_expr    : add_expr '+' mul_expr
            | add_expr '-' mul_expr
            | mul_expr
            ;

mul_expr    : mul_expr '*' unary_expr
            | mul_expr '/' unary_expr
            | mul_expr '%' unary_expr
            | unary_expr
            ;

unary_expr  : '-' unary_expr
            | '!' unary_expr
            | len_expr
            ;

len_expr    : '#' len_expr
            | pow_expr
            ;

pow_expr    : postfix_expr '^' unary_expr    /* -x^2 -> -(x^2) */
            | postfix_expr
            ;

postfix_expr: postfix_expr TOKEN_INC
            | postfix_expr TOKEN_DEC
            | postfix_expr '[' expr ']'
            | postfix_expr '(' opt_list ')'
            | primary_expr
            ;

primary_expr: TOKEN_IDENTIFIER
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
  fprintf(stderr, "line %d: %s\n", yylineno, msg);
  return 0;
}
