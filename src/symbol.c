#include "symbol.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Create a new symbol with the given kind, type, and name.
struct symbol * symbol_create( symbol_t kind, struct type *type, char *name ) {
  struct symbol *s = malloc(sizeof(struct symbol));
  if (!s) {
    fprintf(stderr, "Failed to allocate memory for symbol\n");
    exit(EXIT_FAILURE);
  }
  s->kind = kind;
  s->type = type;
  s->name = name;
  s->which = -1;
  return s;
}

void symbol_print_binding(const struct symbol *s) {
  if (!s) return;
  const char *kind;
  switch (s->kind) {
    case SYMBOL_LOCAL:  kind = "local"; break;
    case SYMBOL_PARAM:  kind = "param"; break;
    case SYMBOL_GLOBAL: kind = "global"; break;
    default: unreachable();
  }
  printf("%s %s %d\n", s->name, kind, s->which);
}
