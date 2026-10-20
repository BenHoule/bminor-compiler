#include "param_list.h"
#include "resolver.h"
#include "scope.h"
#include "symbol.h"
#include "type.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// Create a new parameter list node with the given name, type, and next pointer.
struct param_list * param_list_create( char *name, struct type *type, struct param_list *next ) {
  struct param_list *p = malloc(sizeof(struct param_list));
  if (!p) {
    fprintf(stderr, "Failed to allocate memory for parameter list\n");
    exit(EXIT_FAILURE);
  }
  p->name = name;
  p->type = type;
  p->next = next;
  p->symbol = NULL;
  return p;
}

// Print a parameter list node in a human-readable format.
void param_list_print( struct param_list *a ) {
  printf("(");
  for (; a; a = a->next) {
    printf("%s: ", a->name);
    type_print(a->type);
    if (a->next) printf(", ");
  }
  printf(")");
}

// TODO: Make iterative
void param_list_resolve( struct param_list *a ) {
  int param_index = 0;

  for(; a; a = a->next) {
    //TODO: Add line/col of declaration to error message (use yyerror?)
    if ((a->symbol = scope_lookup_local(a->name)) != NULL) {
      resolver_error(
        "Symbol '%s' already declared in the current scope", a->name
      );
      return;
    }
    a->symbol = symbol_create(SYMBOL_PARAM, a->type, a->name);
    a->symbol->which = param_index++;
    if (!scope_bind(a->name, a->symbol)) {
      resolver_error("Failed to bind '%s'", a->name);
      return;
    }
    symbol_print_binding(a->symbol);
  };
}
