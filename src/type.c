#include "type.h"
#include "param_list.h"
#include "expr.h"

#include <stdio.h>
#include <stdlib.h>

// Create a new type with the given kind, subtype, parameter list, and size.
struct type * type_create( type_t kind, struct type *subtype, struct param_list *params, struct expr *size ) {
  struct type *t = malloc(sizeof(struct type));
  if (!t) {
    fprintf(stderr, "Failed to allocate memory for type\n");
    exit(EXIT_FAILURE);
  }
  t->kind = kind;
  t->subtype = subtype;
  t->params = params;
  t->size = size;
  return t;
}

struct type * type_create_void() {
  return type_create(TYPE_VOID, NULL, NULL, NULL);
}
struct type * type_create_boolean() {
  return type_create(TYPE_BOOLEAN, NULL, NULL, NULL);
}
struct type * type_create_char() {
  return type_create(TYPE_CHAR, NULL, NULL, NULL);
}
struct type * type_create_integer() {
  return type_create(TYPE_INTEGER, NULL, NULL, NULL);
}
struct type * type_create_double() {
  return type_create(TYPE_DOUBLE, NULL, NULL, NULL);
}
struct type * type_create_string() {
  return type_create(TYPE_STRING, NULL, NULL, NULL);
}
struct type * type_create_array(struct type *subtype, struct expr *size) {
  return type_create(TYPE_ARRAY, subtype, NULL, size);
}
struct type * type_create_carray(struct type *subtype, struct expr *size) {
  return type_create(TYPE_CARRAY, subtype, NULL, size);
}
struct type * type_create_function(struct type *subtype, struct param_list *params) {
  return type_create(TYPE_FUNCTION, subtype, params, NULL);
}
struct type * type_create_auto() {
  return type_create(TYPE_AUTO, NULL, NULL, NULL);
}

//TODO: Handle 'auto' (currently treated as its own type)
bool type_equals(struct type *a, struct type *b) {
  if (a == b) return true;
  if (!a || !b || a->kind != b->kind) return false;

  switch (a->kind) {
    case TYPE_ARRAY:
    case TYPE_CARRAY:
      return type_equals(a->subtype, b->subtype);
    case TYPE_FUNCTION:
    if (!type_equals(a->subtype, b->subtype)) return false;
    const struct param_list *p = a->params;
    const struct param_list *q = b->params;
    while(p && q) {
      if (!type_equals(p->type, q->type)) return false;
      p = p->next;
      q = q->next;
      // NOTE: Param names are not compared (standard convention)
    }
    return p == NULL && q == NULL;
    default: return true;
  }
}

// Print a type in a human-readable format.
void type_print( struct type *t ) {
  if (!t) {
    printf("NULL_TYPE");
    return;
  }
  switch (t->kind) {
    case TYPE_VOID:
      printf("void");
      break;
    case TYPE_BOOLEAN:
      printf("boolean");
      break;
    case TYPE_CHAR:
      printf("char");
      break;
    case TYPE_INTEGER:
      printf("integer");
      break;
    case TYPE_DOUBLE:
      printf("double");
      break;
    case TYPE_STRING:
      printf("string");
      break;
    case TYPE_ARRAY:
      printf("array [");
      if (t->size) expr_print(t->size);
      printf("] ");
      type_print(t->subtype);
      break;
    case TYPE_CARRAY:
      printf("carray [");
      if (t->size) expr_print(t->size);
      printf("] ");
      type_print(t->subtype);
      break;
    case TYPE_FUNCTION:
      printf("function ");
      type_print(t->subtype);
      printf(" (");
      if (t->params) {
        param_list_print(t->params);
        printf(" ");
      }
      printf(")");
      break;
    case TYPE_AUTO:
      printf("auto");
      break;
    default: unreachable();
  }
}

void type_resolve(struct type *t) {
  while (t) {
    expr_resolve(t->size);
    t = t->subtype;
  }
}
