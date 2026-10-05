#include "stmt.h"

#include <stdlib.h>

// Create a new statement with the given properties.
struct stmt * stmt_create( stmt_t kind, struct decl *decl, struct expr *init_expr, struct expr *expr, struct expr *next_expr, struct stmt *body, struct stmt *else_body, struct stmt *next ) {
  struct stmt *s = malloc(sizeof(struct stmt));
  s->kind = kind;
  s->decl = decl;
  s->init_expr = init_expr;
  s->expr = expr;
  s->next_expr = next_expr;
  s->body = body;
  s->else_body = else_body;
  s->next = next;
  return s;
}

// Print a statement with the given indentation level.
void stmt_print( struct stmt *s, int indent ) {
  (void)*s;
  (void)indent;
  return;
}
