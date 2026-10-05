#include "type.h"
#include <stdlib.h>

// Create a new type with the given kind, subtype, parameter list, and size.
struct type * type_create( type_t kind, struct type *subtype, struct param_list *params, struct expr *size ) {
  struct type *t = malloc(sizeof(struct type));
  t->kind = kind;
  t->subtype = subtype;
  t->params = params;
  t->size = size;
  return t;
}

// Print a type in a human-readable format.
void type_print( struct type *t ) {
  (void)*t;
  return;
}
