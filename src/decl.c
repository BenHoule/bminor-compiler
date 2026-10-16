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
  if (!d) {
    printf("NULL_DECL");
    return;
  }

  printf("%s: ", d->name);
  type_print(d->type);
  if (d->value) {  // Var initialization
    printf(" = ");
    expr_print(d->value);
  } else if (d->code) {  // Function initialization
    printf(" = ");
    stmt_print(d->code, indent);
    return;
  }
  printf(";");
  if (d->next) {
    printf("\n%*s", indent * 2, "");
    decl_print(d->next, indent);
  }
}

static int local_count = 0;

void decl_resolve( struct decl *d ) {
  //TODO: Add line/col to error message (use yyerror?)
  while (d) {
    struct symbol * pre = scope_lookup_local(d->name);
    if (pre) {
      d->symbol = pre;    // Link to pre-existing symbol
      assert(pre->type);  // All names should have an associated type(?)

      if (pre->type->kind != TYPE_FUNCTION) {
        resolver_error(
          "Variable '%s' already declared in the current scope", d->name);
        d = d->next;
        continue;
      }

      //TODO: Check if function signatures match

      if (d->code == NULL) {
        resolver_error(
          "Function '%s' already declared in the current scope", d->name);
        d = d->next;
        continue;
      }

      //TODO: Check if 'pre' was declaration vs definition (how?)

    } else {  // Create new symbol
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
    if (d->value) expr_resolve(d->value);
    else if (d->code) {
      local_count = 0;  // Reset per function
      scope_enter();
      param_list_resolve(d->type->params);
      stmt_resolve(d->code->block);
      scope_exit();
    }
    d = d->next;
  }
}
