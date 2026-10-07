#include "param_list.h"


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
  return p;
}

// Print a parameter list node in a human-readable format.
void param_list_print( struct param_list *a ) {
  (void)a;
  return;
}
