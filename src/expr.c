#include "expr.h"


#include <stdint.h>
#include <stdlib.h>

// Create a new expression node with the given kind and child expressions.
struct expr * expr_create( expr_t kind, struct expr *left, struct expr *right ) {
  struct expr *e = (struct expr *)malloc(sizeof(struct expr));
  e->kind = kind;
  e->left = left;
  e->right = right;
  e->name = NULL;
  e->int_literal = 0;
  e->double_literal = 0.0;
  e->string_literal = NULL;
  e->symbol = NULL;
  return e;
}

// Wrapper for expr_create(EXPR_NAME, n, NULL)
struct expr * expr_create_name( const char *n ) {
  return expr_create( EXPR_NAME, (struct expr *)n, NULL );
}
// Wrapper for expr_create(EXPR_INTEGER_LITERAL, c, NULL)
struct expr * expr_create_integer_literal( int64_t c ) {
  return expr_create( EXPR_INT, (struct expr *)(intptr_t)c, NULL );
}
// Wrapper for expr_create(EXPR_DOUBLE_LITERAL, c, NULL)
struct expr * expr_create_double_literal( double c ) {
  return expr_create( EXPR_DOUBLE, (struct expr *)(intptr_t)c, NULL );
}
// Wrapper for expr_create(EXPR_BOOLEAN_LITERAL, c, NULL)
struct expr * expr_create_boolean_literal( int c ) {
  return expr_create( EXPR_BOOL, (struct expr *)(intptr_t)c, NULL );
}
// Wrapper for expr_create(EXPR_CHAR_LITERAL, c, NULL)
struct expr * expr_create_char_literal( char c ) {
  return expr_create( EXPR_CHAR, (struct expr *)(intptr_t)c, NULL );
}
// Wrapper for expr_create(EXPR_STRING_LITERAL, str, NULL)
struct expr * expr_create_string_literal( const char *str ) {
  return expr_create( EXPR_STR, (struct expr *)str, NULL );
}

// Print the expression `e` in a human-readable format
void expr_print( struct expr *e ) {
  (void)*e;
  return;
}
