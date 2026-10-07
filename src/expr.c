#include "expr.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
struct expr * expr_create_call( struct expr **argv, size_t argc ) {
  struct expr *e = expr_create( EXPR_CALL, NULL, NULL );
  e->call.argv = argv;
  e->call.argc = argc;
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
  (void)e;
  return;
}
