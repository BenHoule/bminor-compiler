#include "symbol.h"
#include <stdlib.h>

// Create a new symbol with the given kind, type, and name.
struct symbol * symbol_create( symbol_t kind, struct type *type, char *name ) {
  struct symbol *s = malloc(sizeof(struct symbol));
  s->kind = kind;
  s->type = type;
  s->name = name;
  s->which = -1;
  return s;
}
