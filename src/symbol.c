#include "resolver.h"
#include "symbol.h"
#include "memory.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

struct symbol *symbol_create( symbol_t kind, struct type *type, char *name ) {
  struct symbol *s = memory_allocate(sizeof *s);
  s->kind = kind;
  s->type = type;
  s->name = name;
  s->which = -1;
  s->is_defined = false;
  s->frame_locals = 0;
  return s;
}

void symbol_print_binding(const struct symbol *s) {
  if (!s || !resolver_trace) return;
  const char *kind;
  switch (s->kind) {
    case SYMBOL_LOCAL:  kind = "local"; break;
    case SYMBOL_PARAM:  kind = "param"; break;
    case SYMBOL_GLOBAL: kind = "global"; break;
    default: unreachable();
  }
  printf("%s %s %d\n", s->name, kind, s->which);
}
