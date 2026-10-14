#include "expr.h"
#include "scope.h"
#include "string_util.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h> // PRId64 and PRIu64 macros (for printing int64_t)

// Create a new expression node with the given kind and child expressions.
struct expr * expr_create( expr_t kind, struct expr *left, struct expr *right ) {
  struct expr *e = (struct expr *)malloc(sizeof(struct expr));
  if (!e) {
    fprintf(stderr, "Failed to allocate memory for expression\n");
    exit(EXIT_FAILURE);
  }
  e->kind = kind;
  e->left = left;
  e->right = right;
  e->int_literal = 0;
  return e;
}

struct expr * expr_create_name( const char *n ) {
  struct expr *e = expr_create( EXPR_NAME, NULL, NULL );
  e->identifier.name = n;
  e->identifier.symbol = NULL;
  return e;
}
struct expr * expr_create_integer_literal( int64_t i ) {
  struct expr *e = expr_create( EXPR_INT, NULL, NULL );
  e->int_literal = i;
  return e;
}
struct expr * expr_create_double_literal( double d ) {
  struct expr *e = expr_create( EXPR_DOUBLE, NULL, NULL );
  e->double_literal = d;
  return e;
}
struct expr * expr_create_boolean_literal( bool b ) {
  struct expr * e = expr_create( EXPR_BOOL, NULL, NULL );
  e->bool_literal = b;
  return e;
}
struct expr * expr_create_char_literal( char c ) {
  struct expr *e = expr_create( EXPR_CHAR, NULL, NULL );
  e->char_literal = c;
  return e;
}
struct expr * expr_create_string_literal( const char *str ) {
  struct expr *e = expr_create( EXPR_STR, NULL, NULL );
  e->string_literal = str;
  return e;
}
struct expr * expr_create_false() {
  struct expr *e = expr_create( EXPR_BOOL, NULL, NULL );
  e->bool_literal = false;
  return e;
}
struct expr * expr_create_true() {
  struct expr *e = expr_create( EXPR_BOOL, NULL, NULL );
  e->bool_literal = true;
  return e;
}

// Print the expression `e` in a human-readable format
void expr_print( struct expr *e ) {
  if (!e) {
    printf("(NULL_EXPR)");
    return;
  }
  switch (e->kind) {
    case EXPR_ADD:
      printf("(");
      expr_print(e->left);
      printf(" + ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_SUB:
      printf("(");
      expr_print(e->left);
      printf(" - ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_MUL:
      printf("(");
      expr_print(e->left);
      printf(" * ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_DIV:
      printf("(");
      expr_print(e->left);
      printf(" / ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_MOD:
      printf("(");
      expr_print(e->left);
      printf(" %% ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_POW:
      printf("(");
      expr_print(e->left);
      printf(" ^ ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_NEG:
      printf("(-");
      expr_print(e->left);
      printf(")");
      break;
    case EXPR_INC:
      printf("(");
      expr_print(e->left);
      printf("++)");
      break;
    case EXPR_DEC:
      printf("(");
      expr_print(e->left);
      printf("--)");
      break;
    case EXPR_LEN:
      printf("(#");
      expr_print(e->left);
      printf(")");
      break;
    case EXPR_LT:
      printf("(");
      expr_print(e->left);
      printf(" < ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_GT:
      printf("(");
      expr_print(e->left);
      printf(" > ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_LE:
      printf("(");
      expr_print(e->left);
      printf(" <= ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_GE:
      printf("(");
      expr_print(e->left);
      printf(" >= ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_EQ:
      printf("(");
      expr_print(e->left);
      printf(" == ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_NE:
      printf("(");
      expr_print(e->left);
      printf(" != ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_AND:
      printf("(");
      expr_print(e->left);
      printf(" && ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_OR:
      printf("(");
      expr_print(e->left);
      printf(" || ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_NOT:
      printf("(!");
      expr_print(e->left);
      printf(")");
      break;
    case EXPR_ASGN:
      printf("(");
      expr_print(e->left);
      printf(" = ");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_IDX:
      expr_print(e->left);
      printf("[");
      expr_print(e->right);
      printf("]");
      break;
    case EXPR_CALL:
      expr_print(e->left);
      printf("(");
      expr_print(e->right);
      printf(")");
      break;
    case EXPR_ARG:
      expr_print(e->left);
      if (e->right) {
        printf(", ");
        expr_print(e->right);
      }
      break;
    case EXPR_INIT:
      printf("{");
      expr_print(e->left);
      if (e->right) {
        printf(", ");
        expr_print(e->right);
      }
      printf("}");
      break;
    case EXPR_INT:
      printf("%" PRId64, e->int_literal);
      break;
    case EXPR_DOUBLE:
      printf("%.17g", e->double_literal);
      break;
    case EXPR_BOOL:
      printf("%s", e->bool_literal ? "true" : "false");
      break;
    case EXPR_CHAR:
      const char* decoded_char = char_decode(e->char_literal, '\'');
      printf("'%s'", decoded_char);
      break;
    case EXPR_STR:
      static char decoded_str[1278] = "";  // 255(max string length) * 5 + 3
      string_decode(e->string_literal, decoded_str, sizeof(decoded_str));
      printf("%s", decoded_str);
      break;
    case EXPR_NAME:
      printf("%s", e->identifier.name);
      break;
    default: printf("UNKNOWN");
  }
}

void expr_resolve( struct expr *e ) {
  assert(e != NULL);

  //TODO: Add line/col of expression to error message (use yyerror?)
  if (e->kind == EXPR_NAME && (e->identifier.symbol =
      scope_lookup(e->identifier.name)) == NULL) {
    fprintf(stderr, "Error: Undefined symbol '%s'\n", e->identifier.name);
    exit(EXIT_FAILURE);
  }
  if (e->left) expr_resolve(e->left);
  if (e->right) expr_resolve(e->right);
}
