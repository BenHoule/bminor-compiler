#include "scope.h"
#include "hash_table.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

struct scope {
  struct scope *parent;
  struct hash_table *symbols;
};
struct scope *current_scope = NULL;

static int current_depth = 0;

void scope_enter() {
  struct scope *new_scope = (struct scope *)malloc(sizeof(struct scope));
  if (!new_scope) {
    fprintf(stderr, "Failed to allocate memory for new scope\n");
    exit(EXIT_FAILURE);
  }

  new_scope->parent = current_scope;
  new_scope->symbols = hash_table_create(0, 0);
  if (!new_scope->symbols) {
    fprintf(stderr, "Failed to allocate memory for new scope symbols\n");
    free(new_scope);
    exit(EXIT_FAILURE);
  }

  current_scope = new_scope;
  current_depth++;
}

void scope_exit() {
  assert(current_scope != NULL);
  struct scope *old_scope = current_scope;
  current_scope = current_scope->parent;
  hash_table_delete(old_scope->symbols);
  free(old_scope);
  current_depth--;
}

int scope_depth() {
  return current_depth;
}

// Binds name -> symbol  (innermost scope)
int scope_bind(const char *name, struct symbol *symbol ) {
  assert(current_scope != NULL);
  return hash_table_insert(current_scope->symbols, name, symbol);
}

// Searches innermost -> outermost. NULL if not found
struct symbol * scope_lookup( const char *name ) {
  struct scope *scope = current_scope;
  while (scope) {
    struct symbol *sym = hash_table_lookup(scope->symbols, name);
    if (sym) return sym;
    scope = scope->parent;
  }
  return NULL;
}

// Searches only innermost scope. Used for redeclaration checks
struct symbol * scope_lookup_local( const char *name ) {
  assert(current_scope != NULL);
  return hash_table_lookup(current_scope->symbols, name);
}
