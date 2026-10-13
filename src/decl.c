#include "decl.h"
#include "type.h"

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
