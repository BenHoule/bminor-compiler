#include "decl.h"
#include "expr.h"
#include "ir.h"
#include "memory.h"
#include "param_list.h"
#include "resolver.h"
#include "stmt.h"
#include "symbol.h"
#include "scope.h"
#include "type.h"
#include "typecheck.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct decl *decl_create(
  char *name,
  struct type *type, struct expr *value,
  struct stmt *code, struct decl *next
) {
  struct decl *d = memory_allocate(sizeof *d);
  memory_take(name);
  d->name = name;
  d->type = type;
  d->value = value;
  d->code = code;
  d->next = next;
  d->symbol = nullptr;
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

static void decl_resolve_var(struct decl *d) {
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

// WARNING: Early returns prevent bodies of rejected function definitions
// from being resolved; Errors in a rejected body are unreported.
static void decl_resolve_func(struct decl *d) {
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

void decl_resolve(struct decl *d) {
  for(; d; d = d->next) {
    if (d->type->kind == TYPE_FUNCTION) decl_resolve_func(d);
    else decl_resolve_var(d);
  }
}


static void decl_typecheck_variable(struct decl *d) {
  unsigned int old_error_count = typecheck_error_count();

  type_typecheck(d->type);
  if (typecheck_error_count() != old_error_count) return;

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
  } else if (d->value) expr_typecheck_init(d->type, d->value);

  if (typecheck_error_count() != old_error_count) return;

  if (d->value && d->symbol->kind == SYMBOL_GLOBAL) {
    struct expr_constant value;
    if (expr_status(d->value, &value) == EXPR_STAT_RUNTIME) {
      typecheck_error("Global variable '%s' initializer must be constant", d->name);
    }
  }
}

static void decl_typecheck_function(struct decl *d) {
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

void decl_typecheck(struct decl *d) {
  for (struct decl *current = d; current; current = current->next) {
    if (current->type->kind == TYPE_FUNCTION) decl_typecheck_function(current);
    else decl_typecheck_variable(current);
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

bool decl_validate(const struct decl *d) {
  for (; d; d = d->next) {
    if (!d->symbol || !type_validate(d->type) ||
        !type_validate(d->symbol->type) || !expr_validate(d->value) ||
        !stmt_validate(d->code)) return false;
  }
  return true;
}

static int decl_count_params(const struct param_list *p) {
  int n = 0;
  for (; p; p = p->next) n++;
  return n;
}

// Local declarations are lowered into `f`.
// static void decl_lower_local(struct decl *d, struct ir_func *func) {
//   int var = expr_lower_var(d->symbol, func);
//   if (d->value) {
//     int value = expr_lower(d->value, func);
//     ir_emit(func, IR_COPY, var, value, IR_NONE);
//   } else {
//     ir_emit(func, IR_CONST, var, IR_NONE, IR_NONE);  // uninitialized = 0
//   }
// }

static bool decl_is_array(const struct decl *d) {
  assert(d); assert(d->type);
  return d->type->kind == TYPE_ARRAY || d->type->kind == TYPE_CARRAY;
}

// Allocates an array of type `t` into register `dst`, one length per dimension.
static void decl_lower_array(const struct type *t, int dst, struct ir_func *func) {
  assert(t); assert(func);

  int ndims = 0;
  for (const struct type *s = t; s->kind == TYPE_ARRAY || s->kind == TYPE_CARRAY; s = s->subtype)
    ndims++;

  int *dims = memory_allocate((size_t)(ndims) * sizeof *dims);
  int i = 0;
  for (const struct type *s = t; i < ndims; s = s->subtype)
    dims[i++] = expr_lower(s->size, func);

  ir_emit_list(func, IR_ARRAY, dst, dims, ndims);
}

void decl_lower(struct decl *d, struct ir_func *func) {
  int var = expr_lower_var(d->symbol, func);
  if (decl_is_array(d)) {
    decl_lower_array(d->type, var, func);
    if (d->value) expr_lower_init(d->value, var, func);
  } else if (d->value) {
    ir_emit(func, IR_COPY, var, expr_lower(d->value, func), IR_NONE);
  } else {
    ir_emit(func, IR_CONST, var, IR_NONE, IR_NONE);  // uninitialized = 0
  }
}

// Global storage starts zeroed (only initializers/arrays emit code).
static void decl_lower_global(struct decl *d, struct ir_program *prog) {
  d->symbol->which = prog->nglobals++;
  struct ir_func *init = prog->init;

  int value;
  if (decl_is_array(d)) {
    value = ir_new_reg(init);
    decl_lower_array(d->type, value, init);
    if (d->value) expr_lower_init(d->value, value, init);
  } else if (d->value) value = expr_lower(d->value, init);
  else return;

  ir_emit(init, IR_STORE, IR_NONE, ir_emit_global(init, d->symbol), value);
}

struct ir_program *decl_lower_program(struct decl *d) {
  struct ir_program *prog = memory_allocate(sizeof *prog);
  prog->init = ir_func_create("$init", 0, 0);

  for (struct decl *g = d; g; g = g->next)
    if (g->type->kind != TYPE_FUNCTION) decl_lower_global(g, prog);

  ir_emit(prog->init, IR_RETURN, IR_NONE, IR_NONE, IR_NONE);

  struct ir_func **tail = &prog->funcs;
  for (; d; d = d->next) {
    if (d->type->kind != TYPE_FUNCTION || !d->code) continue;

    struct ir_func *func = ir_func_create(
        d->name,
        decl_count_params(d->type->params),
        d->symbol->frame_locals);

    stmt_lower(d->code->block, func);
    ir_emit(func, IR_RETURN, IR_NONE, IR_NONE, IR_NONE);  // falling off the end

    *tail = func;
    tail = &func->next;
  }

  return prog;
}
