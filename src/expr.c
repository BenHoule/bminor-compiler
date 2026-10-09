#include "expr.h"

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
    fprintf(stderr, "(NULL_EXPR)");
    return;
  }

  if (e->kind == EXPR_INIT) {
    fprintf(stderr, "{");
    expr_print(e->left);
    if (e->right) {
      fprintf(stderr, ", ");
      expr_print(e->right);
    }
    fprintf(stderr, "}");
    return;
  }

  fprintf(stderr, "(");
  switch (e->kind) {
    case EXPR_ADD:
      expr_print(e->left);
      fprintf(stderr, " + ");
      expr_print(e->right);
      break;
    case EXPR_SUB:
      expr_print(e->left);
      fprintf(stderr, " - ");
      expr_print(e->right);
      break;
    case EXPR_MUL:
      expr_print(e->left);
      fprintf(stderr, " * ");
      expr_print(e->right);
      break;
    case EXPR_DIV:
      expr_print(e->left);
      fprintf(stderr, " / ");
      expr_print(e->right);
      break;
    case EXPR_MOD:
      expr_print(e->left);
      fprintf(stderr, " %% ");
      expr_print(e->right);
      break;
    case EXPR_NEG:
      fprintf(stderr, "-");
      expr_print(e->left);
      break;
    case EXPR_INC:
      expr_print(e->left);
      fprintf(stderr, "++");
      break;
    case EXPR_DEC:
      expr_print(e->left);
      fprintf(stderr, "--");
      break;
    case EXPR_LEN:
      fprintf(stderr, "#");
      expr_print(e->left);
      break;
    case EXPR_LT:
      expr_print(e->left);
      fprintf(stderr, " < ");
      expr_print(e->right);
      break;
    case EXPR_GT:
      expr_print(e->left);
      fprintf(stderr, " > ");
      expr_print(e->right);
      break;
    case EXPR_LE:
      expr_print(e->left);
      fprintf(stderr, " <= ");
      expr_print(e->right);
      break;
    case EXPR_GE:
      expr_print(e->left);
      fprintf(stderr, " >= ");
      expr_print(e->right);
      break;
    case EXPR_EQ:
      expr_print(e->left);
      fprintf(stderr, " == ");
      expr_print(e->right);
      break;
    case EXPR_NE:
      expr_print(e->left);
      fprintf(stderr, " != ");
      expr_print(e->right);
      break;
    case EXPR_AND:
      expr_print(e->left);
      fprintf(stderr, " && ");
      expr_print(e->right);
      break;
    case EXPR_OR:
      expr_print(e->left);
      fprintf(stderr, " || ");
      expr_print(e->right);
      break;
    case EXPR_NOT:
      fprintf(stderr, "!");
      expr_print(e->left);
      break;
    case EXPR_ASGN:
      expr_print(e->left);
      fprintf(stderr, " = ");
      expr_print(e->right);
      break;
    case EXPR_IDX:
      expr_print(e->left);
      fprintf(stderr, "[");
      expr_print(e->right);
      fprintf(stderr, "]");
      break;
    case EXPR_CALL:
      expr_print(e->left);
      fprintf(stderr, "(");
      expr_print(e->right);
      fprintf(stderr, ")");
      break;
    case EXPR_POW:
      expr_print(e->left);
      fprintf(stderr, " ^ ");
      expr_print(e->right);
      break;
    case EXPR_ARG:
      expr_print(e->left);
      if (e->right) {
        fprintf(stderr, ", ");
        expr_print(e->right);
      }
      break;
    case EXPR_INT:
      fprintf(stderr, "%" PRId64, e->int_literal);
      break;
    case EXPR_DOUBLE:
      fprintf(stderr, "%f", e->double_literal);
      break;
    case EXPR_BOOL:
      fprintf(stderr, "%s", e->bool_literal ? "true" : "false");
      break;
    case EXPR_CHAR:
      fprintf(stderr, "'%c'", e->char_literal);
      break;
    case EXPR_STR:
      fprintf(stderr, "\"%s\"", e->string_literal);
      break;
    case EXPR_NAME:
      fprintf(stderr, "%s", e->identifier.name);
      break;
    default:
      fprintf(stderr, "UNKNOWN");
      break;
  }
  fprintf(stderr, ")");
}
