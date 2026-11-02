#include "type.h"
#include "memory.h"
#include "expr.h"
#include "param_list.h"
#include "typecheck.h"

#include <assert.h>
#include <stdio.h>

struct type *type_create(type_t kind, struct type *subtype, struct param_list *params, struct expr *size) {
  struct type *t = memory_allocate(sizeof *t);
  t->kind = kind;
  t->subtype = subtype;
  t->params = params;
  t->size = size;
  return t;
}

struct type *type_create_void(void) {
  return type_create(TYPE_VOID, nullptr, nullptr, nullptr);
}
struct type *type_create_boolean(void) {
  return type_create(TYPE_BOOLEAN, nullptr, nullptr, nullptr);
}
struct type *type_create_char(void) {
  return type_create(TYPE_CHAR, nullptr, nullptr, nullptr);
}
struct type *type_create_integer(void) {
  return type_create(TYPE_INTEGER, nullptr, nullptr, nullptr);
}
struct type *type_create_double(void) {
  return type_create(TYPE_DOUBLE, nullptr, nullptr, nullptr);
}
struct type *type_create_string(void) {
  return type_create(TYPE_STRING, nullptr, nullptr, nullptr);
}
struct type *type_create_array(struct type *subtype, struct expr *size) {
  return type_create(TYPE_ARRAY, subtype, nullptr, size);
}
struct type *type_create_carray(struct type *subtype, struct expr *size) {
  return type_create(TYPE_CARRAY, subtype, nullptr, size);
}
struct type *type_create_function(struct type *subtype, struct param_list *params) {
  return type_create(TYPE_FUNCTION, subtype, params, nullptr);
}
struct type *type_create_auto(void) {
  return type_create(TYPE_AUTO, nullptr, nullptr, nullptr);
}

struct type *type_primitive(type_t kind) {
  assert(kind >= TYPE_VOID && kind <= TYPE_STRING);
  static struct type primitives[] = {
    {.kind = TYPE_VOID},
    {.kind = TYPE_BOOLEAN},
    {.kind = TYPE_CHAR},
    {.kind = TYPE_INTEGER},
    {.kind = TYPE_DOUBLE},
    {.kind = TYPE_STRING}
  };
  return &primitives[kind];
}

// Array lengths and parameter names do not participate in compatibility.
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
        // Param names are not compared (standard convention)
      }

      return p == nullptr && q == nullptr;

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
    if (!s) continue; // Avoid double-reporting

    if (s->kind != TYPE_INTEGER) {
      typecheck_error("Array size must be an integer");
      continue;
    }

    struct expr_constant size;
    const bool size_is_known =
      expr_status(t->size, &size) == EXPR_STAT_CONST;

    if (size_is_known && size.integer <= 0)
      typecheck_error("Array size must be a positive integer");
  }
}

bool type_validate(const struct type *t) {
  if (!t || t->kind == TYPE_AUTO) return false;
  if (t->kind == TYPE_ARRAY || t->kind == TYPE_CARRAY)
    return type_validate(t->subtype) && (!t->size || expr_validate(t->size));

  if (t->kind == TYPE_FUNCTION) {
    if (!type_validate(t->subtype)) return false;

    for (const struct param_list *p = t->params; p; p = p->next)
      if (!type_validate(p->type)) return false;

    return true;
  }

  return t->kind >= TYPE_VOID && t->kind <= TYPE_STRING;
}
