#include "decl.h"
#include "param_list.h"
#include "type.h"
#include "expr.h"
#include "stmt.h"
#include "symbol.h"
#include "scope.h"

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

void decl_resolve( struct decl *d ) {
  int stack_index = 0;

  while (d) {
    //TODO: Add line/col of declaration to error message (use yyerror?)
    if (scope_lookup_local(d->name)) {
      fprintf(stderr,
        "Symbol '%s' already declared in the current scope\n",
        d->name
      );
      exit(EXIT_FAILURE);
    }

    symbol_t kind = (d->code) ? SYMBOL_LOCAL : SYMBOL_GLOBAL;
    d->symbol = symbol_create(kind, d->type, d->name);
    if (kind == SYMBOL_LOCAL) d->symbol->which = stack_index++;
    scope_bind(d->name, d->symbol);

    if (d->value) expr_resolve(d->value);
    else if (d->code) {
      scope_enter();
      param_list_resolve(d->type->params);
      stmt_resolve(d->code);
      scope_exit();
    }

    d = d->next;
  }
}
