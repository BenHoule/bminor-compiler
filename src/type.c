#include "type.h"
#include "expr.h"
#include "param_list.h"
#include "typecheck.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct type *type_create(type_t kind, struct type *subtype, struct param_list *params, struct expr *size) {
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

struct type *type_create_void(void) {
  return type_create(TYPE_VOID, NULL, NULL, NULL);
}
struct type *type_create_boolean(void) {
  return type_create(TYPE_BOOLEAN, NULL, NULL, NULL);
}
struct type *type_create_char(void) {
  return type_create(TYPE_CHAR, NULL, NULL, NULL);
}
struct type *type_create_integer(void) {
  return type_create(TYPE_INTEGER, NULL, NULL, NULL);
}
struct type *type_create_double(void) {
  return type_create(TYPE_DOUBLE, NULL, NULL, NULL);
}
struct type *type_create_string(void) {
  return type_create(TYPE_STRING, NULL, NULL, NULL);
}
struct type *type_create_array(struct type *subtype, struct expr *size) {
  return type_create(TYPE_ARRAY, subtype, NULL, size);
}
struct type *type_create_carray(struct type *subtype, struct expr *size) {
  return type_create(TYPE_CARRAY, subtype, NULL, size);
}
struct type *type_create_function(struct type *subtype, struct param_list *params) {
  return type_create(TYPE_FUNCTION, subtype, params, NULL);
}
struct type *type_create_auto(void) {
  return type_create(TYPE_AUTO, NULL, NULL, NULL);
}

struct type *type_primitive(type_t kind) {
  static struct type primitives[TYPE_AUTO + 1] = {0};
  primitives[kind].kind = kind;
  return &primitives[kind];
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

static void type_print_affix(const char* prefix, struct type *t, const char* suffix) {
  if (prefix && *prefix != '\0') printf("%s", prefix);
  type_print(t);
  if (suffix && *suffix != '\0') printf("%s", suffix);
}

void type_print(struct type *t) {
  switch (t->kind) {
    case TYPE_VOID:    printf("void"   ); break;
    case TYPE_BOOLEAN: printf("boolean"); break;
    case TYPE_CHAR:    printf("char"   ); break;
    case TYPE_INTEGER: printf("integer"); break;
    case TYPE_DOUBLE:  printf("double" ); break;
    case TYPE_STRING:  printf("string" ); break;
    case TYPE_AUTO:    printf("auto"   ); break;
    case TYPE_CARRAY:  printf("c"); [[fallthrough]];
    case TYPE_ARRAY:
      assert(t->subtype);
      expr_print_affix("array [", t->size, "] ");
      type_print(t->subtype);
      break;
    case TYPE_FUNCTION:
      assert(t->subtype);
      type_print_affix("function ", t->subtype, " ");
      param_list_print(t->params);
      break;
    default: unreachable();
  }
}

void type_resolve(struct type *t) {
  for (;t; t = t->subtype) expr_resolve(t->size);
}

bool type_is_primitive(const struct type *t) {
  if (!t) return false;
  switch (t->kind) {
    case TYPE_BOOLEAN:
    case TYPE_CHAR:
    case TYPE_INTEGER:
    case TYPE_DOUBLE:
    case TYPE_STRING:
      return true;
    default:
      return false;
  }
}

void type_typecheck(struct type *t) {
  for (; t; t = t->subtype) {
    if (!t->size) continue;

    struct type *s = expr_typecheck(t->size);
    if (s && s->kind != TYPE_INTEGER) {
      typecheck_error("Array size must be an integer");
    } else if (t->size->kind == EXPR_NEG || t->size->int_literal <= 0) {
      // Above check blocks [-(-1)] ^^^
      // TODO: Evaluate const expressions to handle negative literals correctly
      typecheck_error("Array size must be a positive integer");
    }
  }
}
