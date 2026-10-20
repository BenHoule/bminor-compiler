#include "decl.h"
#include "expr.h"
#include "param_list.h"
#include "resolver.h"
#include "stmt.h"
#include "symbol.h"
#include "scope.h"
#include "type.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

// Create a new declaration node with the given properties.
struct decl * decl_create( char *name, struct type *type, struct expr *value, struct stmt *code, struct decl *next ) {
  struct decl *d = (struct decl *)malloc(sizeof(struct decl));
  if (!d) {
    fprintf(stderr, "Failed to allocate memory for declaration\n");
    exit(EXIT_FAILURE);
  }
  d->name = name;
  d->type = type;
  d->value = value;
  d->code = code;
  d->next = next;
  d->symbol = NULL;
  return d;
}

// Print the declaration `d` with the given indentation level.
void decl_print( struct decl *d, int indent ) {
  for (; d; d = d->next) {
    printf("%s: ", d->name);
    type_print(d->type);
    if (d->value) {  // Var
      printf(" = ");
      expr_print(d->value);
    } else if (d->code) {  // Function
      printf(" = ");
      stmt_print(d->code, indent);
      return;
    }
    printf(";");
    if (d->next) {
      printf("\n%*s", indent * 2, "");
    }
  }
}

static int local_count = 0;

static void decl_resolve_var(struct decl *d) {
  assert(d); assert(d->type);  // Everything has a type

  // Resolving initializer before binding var prevents self-reference.
  expr_resolve(d->value);
  type_resolve(d->type);

  //TODO: Add line/col to error messages
  if (scope_lookup_local(d->name)) {
    resolver_error(
      "Variable '%s' already declared in current scope", d->name);
    return;
  }

  symbol_t kind = (scope_depth() > 1) ? SYMBOL_LOCAL : SYMBOL_GLOBAL;
  d->symbol = symbol_create(kind, d->type, d->name);
  d->symbol->which = (kind == SYMBOL_LOCAL) ? local_count++ : -1;
  if (!scope_bind(d->name, d->symbol)) {
    resolver_error(
      "Failed to bind '%s'", d->name);
    return;
  }
  symbol_print_binding(d->symbol);
}

static void decl_resolve_func(struct decl *d) {
  assert(d); assert(d->type); assert(d->type->kind == TYPE_FUNCTION);

  //TODO: Add line/col to error messages
  if (scope_depth() > 1) {
    resolver_error(
      "Function '%s' declared in a local scope (must be global)", d->name);
    return;
  }

  struct symbol * pre = scope_lookup_local(d->name);
  if (pre) {
    // NOTE: Early returns prevent bodies of rejected function definitions
    // from being resolved; Errors in a rejected body are unreported.
    if (pre->type->kind != TYPE_FUNCTION) {
      resolver_error(
        "Function '%s' already declared (as a variable)", d->name);
      return;
    }
    if (!type_equals(pre->type, d->type)) {
      resolver_error(
        "Function '%s' declared with different types", d->name);
      return;
    }
    if (d->code && pre->is_defined) {
      resolver_error("Function '%s' already defined", d->name);
      return;
    }
    d->symbol = pre;
  } else {
    d->symbol = symbol_create(SYMBOL_GLOBAL, d->type, d->name);
    if (!scope_bind(d->name, d->symbol)) {
      resolver_error("Failed to bind '%s'", d->name);
      return;
    }
    symbol_print_binding(d->symbol);
  }

  if (!d->code) return;

  d->symbol->is_defined = true;  // Set before resolving so recursion works
  local_count = 0;
  scope_enter();
  param_list_resolve(d->type->params);
  stmt_resolve(d->code->block);
  scope_exit();
}

void decl_resolve( struct decl *d ) {
  for(; d; d = d->next) {
    if (d->type->kind == TYPE_FUNCTION) decl_resolve_func(d);
    else decl_resolve_var(d);
  }
}
