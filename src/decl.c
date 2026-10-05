#include "decl.h"

#include <stdlib.h>

// Create a new declaration node with the given properties.
struct decl * decl_create( char *name, struct type *type, struct expr *value, struct stmt *code, struct decl *next ) {
  struct decl *d = (struct decl *)malloc(sizeof(struct decl));
  d->name = name;
  d->type = type;
  d->value = value;
  d->code = code;
  d->next = next;
  return d;
}

// Print the declaration `d` with the given indentation level.
void decl_print( struct decl *d, int indent ) {
  (void)*d;
  (void)indent;
  return;
}
