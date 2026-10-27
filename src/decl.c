#include "decl.h"
#include "expr.h"
#include "param_list.h"
#include "resolver.h"
#include "stmt.h"
#include "symbol.h"
#include "scope.h"
#include "type.h"
#include "typecheck.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct decl *decl_create(char *const name, struct type *const type, struct expr *const value,
                          struct stmt *const code, struct decl *const next) {
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

void decl_print(const struct decl *d, const int indent) {
  for (; d; d = d->next) {
    printf("%s: ", d->name);
    type_print(d->type);

    if (d->value) {  // Var initializer
      printf(" = ");
      expr_print(d->value);
      printf(";");
    } else if (d->code) {  // Function definition
      printf(" = ");
      stmt_print(d->code, indent);
    } else printf(";");  // Declaration

    if (d->next) {
      printf("\n%*s", indent * 2, "");
    }
  }
}

static int local_count = 0;

static void decl_resolve_var(struct decl *const d) {
  assert(d); assert(d->type);  // Everything has a type

  // Resolving initializer before binding var prevents self-reference.
  expr_resolve(d->value);
  type_resolve(d->type);

  // Check if `d->name` is already bound.
  //TODO: Add line/col to error messages
  if (scope_lookup_local(d->name)) {
    resolver_error(
      "Variable '%s' already declared in current scope", d->name);
    return;
  }

  // Create + bind new symbol to `d->name`
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

// NOTE: Early returns prevent bodies of rejected function definitions
// from being resolved; Errors in a rejected body are unreported.
static void decl_resolve_func(struct decl *const d) {
  assert(d); assert(d->type); assert(d->type->kind == TYPE_FUNCTION);

  //TODO: Add line/col to error messages
  if (scope_depth() > 1) {
    resolver_error(
      "Function '%s' declared in a local scope (must be global)", d->name);
    return;
  }

  struct symbol *pre = scope_lookup_local(d->name);
  if (pre) {
    // Existing symbol found: Check for conflicts
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
    d->symbol = pre; // No conflicts: Reuse existing symbol

  } else {
    // No existing symbol: Create new one for the function
    d->symbol = symbol_create(SYMBOL_GLOBAL, d->type, d->name);
    if (!scope_bind(d->name, d->symbol)) {
      resolver_error("Failed to bind '%s'", d->name);
      return;
    }

    symbol_print_binding(d->symbol);
  }
  if (!d->code) return;  // Declaration (no body to resolve)

  d->symbol->is_defined = true;  // Set before resolving so recursion works
  local_count = 0;
  scope_enter();
  param_list_resolve(d->type->params);
  stmt_resolve(d->code->block);
  scope_exit();
  d->symbol->frame_locals = local_count;
}

void decl_resolve( struct decl *d ) {
  for(; d; d = d->next) {
    if (d->type->kind == TYPE_FUNCTION) decl_resolve_func(d);
    else decl_resolve_var(d);
  }
}

static void decl_typecheck_variable(struct decl *const d) {
  type_typecheck(d->type);
  if (d->value && d->symbol->kind == SYMBOL_GLOBAL && !expr_is_constant(d->value)) {
    typecheck_error("Global variable '%s' initializer must be constant", d->name);
    return;
  }

  if (d->type->kind == TYPE_AUTO) {
    if (!d->value || d->value->kind == EXPR_INIT) {
      typecheck_error("Cannot infer type for '%s'", d->name);
      return;
    }

    struct type *t = expr_typecheck(d->value);
    if (!t) return; // Avoid double-reporting invalid expression
    if (!type_is_primitive(t)) {
        typecheck_error("Cannot infer type for '%s' (non-primitive)", d->name);
        return;
    }

    d->type = d->symbol->type = t;
    return;
  }
  if (d->value) expr_typecheck_init(d->type, d->value);
}

static void decl_typecheck_function(struct decl *const d) {
  struct type *ftype = d->symbol->type;
  if (d->code) {
    // Function definition: Typecheck body and infer return type (if needed).
    stmt_typecheck(d->code->block, ftype);
    if (ftype->subtype->kind == TYPE_AUTO) {
      ftype->subtype = type_primitive(TYPE_VOID);
    }

    d->type->subtype = ftype->subtype;
  }

  if (strcmp(d->name, "main") == 0 && ftype->subtype->kind != TYPE_INTEGER)
    typecheck_error("'main' must return an integer");
}

void decl_typecheck( struct decl *d ) {
  for (struct decl *current = d; current; current = current->next) {
    if (current->type->kind == TYPE_FUNCTION) decl_typecheck_function(d);
    else decl_typecheck_variable(d);
  }

  // Second loop lets later definition resolve earlier prototype's return type
  for (struct decl *current = d; current; current = current->next) {
    if (current->type->kind != TYPE_FUNCTION) continue;

    struct type *return_type = current->symbol->type->subtype;
    if (return_type->kind == TYPE_AUTO) {
      typecheck_error(
        "Cannot infer return type for function '%s'; "
        "provide a definition or an explicit return type",
        current->name
      );
    }

    current->type->subtype = return_type;
  }
}
