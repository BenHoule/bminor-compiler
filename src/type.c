#include "type.h"
#include "param_list.h"

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
struct type * type_create_character() {
  return type_create(TYPE_CHARACTER, NULL, NULL, NULL);
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

// Print a type in a human-readable format.
void type_print( struct type *t ) {
  if (!t) {
    fprintf(stderr, "NULL_TYPE");
  }
  switch (t->kind) {
    case TYPE_VOID:
      fprintf(stderr, "void");
      break;
    case TYPE_BOOLEAN:
      fprintf(stderr, "boolean");
      break;
    case TYPE_CHARACTER:
      fprintf(stderr, "character");
      break;
    case TYPE_INTEGER:
      fprintf(stderr, "integer");
      break;
    case TYPE_DOUBLE:
      fprintf(stderr, "double");
      break;
    case TYPE_STRING:
      fprintf(stderr, "string");
      break;
    case TYPE_ARRAY:
      fprintf(stderr, "array[");
      type_print(t->subtype);
      fprintf(stderr, "]");
      break;
    case TYPE_CARRAY:
      fprintf(stderr, "carray[");
      type_print(t->subtype);
      fprintf(stderr, "]");
      break;
    case TYPE_FUNCTION:
      fprintf(stderr, "function ");
      type_print(t->subtype);
      fprintf(stderr, " ( ");
      param_list_print(t->params);
      fprintf(stderr, " )");
      break;
    case TYPE_AUTO:
      fprintf(stderr, "auto");
      break;
    default:
      fprintf(stderr, "UNKNOWN_TYPE");
      break;
  }
}
