#include "expr.h"
#include "ir.h"
#include "memory.h"
#include "param_list.h"
#include "resolver.h"
#include "scope.h"
#include "string_util.h"
#include "symbol.h"
#include "type.h"
#include "typecheck.h"

#include <assert.h>
#include <inttypes.h> // PRId64 and PRIu64 macros (for printing int64_t)
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct expr *expr_create(expr_t kind, struct expr *left, struct expr *right) {
  struct expr *e = memory_allocate(sizeof *e);
  e->kind = kind;
  e->left = left;
  e->right = right;
  e->type = nullptr;
  e->int_literal = 0;
  return e;
}
struct expr *expr_create_name(const char *n) {
  struct expr *e = expr_create( EXPR_NAME, nullptr, nullptr );
  e->name = n ? memory_copy_string(n) : nullptr;
  e->symbol = nullptr;
  return e;
}
struct expr *expr_create_integer_literal(int64_t i) {
  struct expr *e = expr_create( EXPR_INT, nullptr, nullptr );
  e->int_literal = i;
  return e;
}
struct expr *expr_create_double_literal(double d) {
  struct expr *e = expr_create( EXPR_DOUBLE, nullptr, nullptr );
  e->double_literal = d;
  return e;
}
struct expr *expr_create_boolean_literal(bool b) {
  struct expr *e = expr_create( EXPR_BOOL, nullptr, nullptr );
  e->bool_literal = b;
  return e;
}
struct expr *expr_create_char_literal(char c) {
  struct expr *e = expr_create( EXPR_CHAR, nullptr, nullptr );
  e->char_literal = c;
  return e;
}
struct expr *expr_create_string_literal(const char *str) {
  struct expr *e = expr_create( EXPR_STR, nullptr, nullptr );
  e->string_literal = str ? memory_copy_string(str) : nullptr;
  return e;
}
struct expr *expr_create_false(void) {
  struct expr *e = expr_create( EXPR_BOOL, nullptr, nullptr );
  e->bool_literal = false;
  return e;
}
struct expr *expr_create_true(void) {
  struct expr *e = expr_create( EXPR_BOOL, nullptr, nullptr );
  e->bool_literal = true;
  return e;
}


static void expr_print_binop( struct expr *e, const char *op) {
  printf("(");
  expr_print(e->left);
  printf(" %s ", op);
  expr_print(e->right);
  printf(")");
}

void expr_print_affix(const char *prefix, struct expr *e, const char *suffix) {
  if (prefix && *prefix != '\0') printf("%s", prefix);
  expr_print(e);
  if (suffix && *suffix != '\0') printf("%s", suffix);
}

// Print the expression `e` in a human-readable format
void expr_print(struct expr *e) {
  if (!e) return;

  //TODO: Make precedence-aware (only use '()' when precedence <= parent's)
  switch (e->kind) {
    case EXPR_ASGN:expr_print_binop(e, "="); break;
    case EXPR_ADD: expr_print_binop(e, "+"); break;
    case EXPR_SUB: expr_print_binop(e, "-"); break;
    case EXPR_MUL: expr_print_binop(e, "*"); break;
    case EXPR_DIV: expr_print_binop(e, "/"); break;
    case EXPR_MOD: expr_print_binop(e, "%"); break;
    case EXPR_POW: expr_print_binop(e, "^"); break;
    case EXPR_LT:  expr_print_binop(e, "<"); break;
    case EXPR_GT:  expr_print_binop(e, ">"); break;
    case EXPR_GE:  expr_print_binop(e, ">="); break;
    case EXPR_LE:  expr_print_binop(e, "<="); break;
    case EXPR_EQ:  expr_print_binop(e, "=="); break;
    case EXPR_NE:  expr_print_binop(e, "!="); break;
    case EXPR_OR:  expr_print_binop(e, "||"); break;
    case EXPR_AND: expr_print_binop(e, "&&"); break;
    case EXPR_NEG: expr_print_affix("(-", e->left, ")"); break;
    case EXPR_LEN: expr_print_affix("(#", e->left, ")"); break;
    case EXPR_NOT: expr_print_affix("(!", e->left, ")"); break;
    case EXPR_INC: expr_print_affix("((", e->left, ")++)"); break;
    case EXPR_DEC: expr_print_affix("((", e->left, ")--)"); break;
    case EXPR_NAME:   printf("%s", e->name  ); break;
    case EXPR_INT:    printf("%" PRId64, e->int_literal); break;
    case EXPR_DOUBLE: printf("%#.17g", e->double_literal); break;
    case EXPR_BOOL:   printf("%s", e->bool_literal ? "true" : "false"); break;
    case EXPR_CHAR: {
      char decoded[6];
      char_decode(e->char_literal, '\'', decoded);
      printf("'%s'", decoded);
      break;
    }
    case EXPR_STR:
      char decoded_str[1278];  // 255(max string length) * 5 + 3
      size_t decoded_size = string_decode(e->string_literal, decoded_str, sizeof decoded_str);
      if (decoded_size == ERR_TOO_LONG || decoded_size == ERR_INVALID_CHAR) {
        fprintf(stderr, "bminor: cannot print string literal\n");
        exit(EXIT_FAILURE);
      }
      printf("%s", decoded_str);
      break;

    case EXPR_IDX:
      expr_print(e->left);
      expr_print_affix("[", e->right, "]");
      break;

    case EXPR_CALL:
      expr_print(e->left);
      expr_print_affix("(", e->right, ")");
      break;

    case EXPR_ARG:
      expr_print(e->left);
      if (e->right) printf(", ");
      expr_print(e->right);
      break;

    case EXPR_INIT:
      printf("{");
      expr_print(e->left);
      if (e->right) printf(", ");
      expr_print(e->right);
      printf("}");
      break;

    default: unreachable();
  }
}


/**
 * Resolves names using an explicit stack without modifying AST links.
 *
 * Morris traversal is a possible O(1)-auxiliary-space alternative, but it
 * temporarily threads AST pointers and requires exclusive access and complete
 * traversal. See the "Expression resolver traversal" section in src/README.md
 * for a copyable implementation and its invariants.
 */
void expr_resolve( struct expr *e ) {
  if (!e) return;
  size_t count = 0, capacity = 32;
  struct expr **stack = malloc(capacity * sizeof *stack);
  if (!stack) {
    fprintf(stderr, "bminor: failed to allocate resolver stack\n");
    exit(EXIT_FAILURE);
  }

  stack[count++] = e;
  while (count) {
    e = stack[--count];
    if (e->kind == EXPR_NAME && !(e->symbol = scope_lookup(e->name)))
      resolver_error("Undefined identifier '%s'", e->name);

    if (count + 2 > capacity) {
      capacity *= 2;
      struct expr **grown = realloc(stack, capacity * sizeof *stack);
      if (!grown) {
        free(stack);
        fprintf(stderr, "bminor: failed to grow resolver stack\n");
        exit(EXIT_FAILURE);
      }

      stack = grown;
    }

    if (e->right) stack[count++] = e->right;
    if (e->left) stack[count++] = e->left;
  }

  free(stack);
}

static const char *expr_op_name(expr_t kind) {
  switch (kind) {
    case EXPR_ADD: return "+";   case EXPR_SUB: return "-";
    case EXPR_MUL: return "*";   case EXPR_DIV: return "/";
    case EXPR_MOD: return "%";   case EXPR_POW: return "^";
    case EXPR_NEG: return "-";   case EXPR_NOT: return "!";
    case EXPR_INC: return "++";  case EXPR_DEC: return "--";
    case EXPR_LEN: return "#";   case EXPR_LT:  return "<";
    case EXPR_GT:  return ">";   case EXPR_LE:  return "<=";
    case EXPR_GE:  return ">=";  case EXPR_EQ:  return "==";
    case EXPR_NE:  return "!=";  case EXPR_AND: return "&&";
    case EXPR_OR:  return "||";  case EXPR_ASGN: return "=";
    default: return "?";
  }
}


static bool expr_is_lvalue(const struct expr *e) {
  if (!e) return false;

  const bool is_assignable = e->kind == EXPR_NAME && e->symbol
                             && e->symbol->type->kind != TYPE_FUNCTION
                             && e->symbol->type->kind != TYPE_VOID;

  return e->kind == EXPR_IDX || is_assignable;
}

static struct type *expr_typecheck_binary(struct expr *e, bool allow_non_numeric) {
  if (!e) return nullptr;

  struct type *left_type = expr_typecheck(e->left);
  struct type *right_type = expr_typecheck(e->right);
  if (!left_type || !right_type) return nullptr;
  if (!type_equals(left_type, right_type)) {
    typecheck_error("'%s' requires matching operand types "
                    "(no implicit conversion)",
                    expr_op_name(e->kind));
    return nullptr;
  }

  switch (left_type->kind) {
    case TYPE_INTEGER: break;
    case TYPE_DOUBLE:
      if (e->kind == EXPR_MOD || e->kind == EXPR_POW) {  // Matches C/C++
        typecheck_error("%s cannot operate on double operands",
                        expr_op_name(e->kind));
        return nullptr;
      }  // Could check type on code generation & use fmod()/pow() for doubles
      break;

    case TYPE_VOID: case TYPE_ARRAY: case TYPE_CARRAY: case TYPE_FUNCTION:
      typecheck_error("'%s' cannot operate on non-primitive types",
                      expr_op_name(e->kind));
      return nullptr;

    default:
      if (allow_non_numeric) break;
      typecheck_error("'%s' requires numeric operands",
                      expr_op_name(e->kind));
      return nullptr;
  }

  switch (e->kind) {
    case EXPR_ADD: case EXPR_SUB: case EXPR_MUL:
    case EXPR_DIV: case EXPR_MOD: case EXPR_POW: return left_type;

    default: return type_primitive(TYPE_BOOLEAN);
  }
}

static struct type *expr_typecheck_logical(struct expr *e) {
  if (!e) return nullptr;

  e->type = expr_typecheck(e->left);
  struct type *right_type = expr_typecheck(e->right);
  if (!e->type || !right_type) return nullptr;  // Avoid double-reporting

  if (e->type->kind != TYPE_BOOLEAN || right_type->kind != TYPE_BOOLEAN) {
    typecheck_error("logical operator '%s' requires boolean operands",
                    expr_op_name(e->kind));
    return nullptr;
  }

  return type_primitive(TYPE_BOOLEAN);
}

static struct type *expr_typecheck_call(struct expr *e) {
  if (!e) return nullptr;

  struct type *t = expr_typecheck(e->left);
  if (!t) return nullptr;  // Avoid double-reporting
  if (t->kind != TYPE_FUNCTION) {
    typecheck_error("call requires a function");
    return nullptr;
  }
  if (t->subtype->kind == TYPE_AUTO) {
    typecheck_error(
      "cannot call a function before its return type is inferred; "
      "use an explicit return type"
    );
    return nullptr;
  }

  bool valid = true;
  bool previously_reported = false;
  int index = 1;
  const struct param_list *param = t->params;

  for (struct expr *arg = e->right; arg; arg = arg->right, index++) {
    if (!param) {
      if (!previously_reported) {
        typecheck_error("too many arguments in function call");
        previously_reported = true;
      }
      valid = false;
      continue;
    }

    struct type *arg_type = expr_typecheck(arg->left);
    if (!arg_type) valid = false;
    else if (!type_equals(arg_type, param->type)) {
      typecheck_error("argument %d has incorrect type", index);
      valid = false;
    }

    param = param->next;
  }

  if (param) {
    typecheck_error("too few arguments in function call");
    return nullptr;
  }
  return valid ? t->subtype : nullptr;
}


// Returns nullptr when `e` contains an error that has already been reported
struct type *expr_typecheck(struct expr *e) {
  if (!e) return nullptr;
  e->type = nullptr;

  switch (e->kind) {
    case EXPR_ADD: case EXPR_SUB: case EXPR_MUL: case EXPR_DIV: case EXPR_MOD:
    case EXPR_POW: case EXPR_LT:  case EXPR_GT:  case EXPR_LE:  case EXPR_GE:
      e->type = expr_typecheck_binary(e, false);
      break;

    case EXPR_EQ:  case EXPR_NE:
      e->type = expr_typecheck_binary(e, true);
      break;

    case EXPR_AND:  case EXPR_OR:
      e->type = expr_typecheck_logical(e);
      break;

    case EXPR_NOT:
      e->type = expr_typecheck(e->left);
      if (e->type && e->type->kind != TYPE_BOOLEAN) {
        typecheck_error("logical NOT ('!') requires a boolean operand");
        e->type = nullptr;
      }
      break;

    case EXPR_NEG:
      e->type = expr_typecheck(e->left);
      if (e->type && e->type->kind != TYPE_INTEGER &&
          e->type->kind != TYPE_DOUBLE) {
        typecheck_error("negation ('-') requires a numeric operand");
        e->type = nullptr;
      }
      break;

    case EXPR_INC:  case EXPR_DEC:
      e->type = expr_typecheck(e->left);
      if (!e->type) break;  // Avoid double-reporting
      if (!expr_is_lvalue(e->left)) {
        typecheck_error("'%s' requires an lvalue",
                        expr_op_name(e->kind));
        e->type = nullptr;
      } else if (e->type->kind != TYPE_INTEGER) {
        typecheck_error("'%s' requires an integer",
                        expr_op_name(e->kind));
        e->type = nullptr;
      }
      break;

    case EXPR_LEN:
      e->type = expr_typecheck(e->left);
      if (!e->type) break;  // Avoid double-reporting
      if (e->type->kind != TYPE_ARRAY) {
        typecheck_error("length operator ('#') requires an array");
        e->type = nullptr;
      } else e->type = type_primitive(TYPE_INTEGER);
      break;

    case EXPR_ASGN:
      e->type = expr_typecheck(e->left);
      if (e->type && !expr_is_lvalue(e->left)) {
        typecheck_error("left side of '=' is not assignable");
        e->type = nullptr;
        // No break (still need to typecheck right-hand side)
      }

      struct type *r = expr_typecheck(e->right);
      if (!r) e->type = nullptr;
      if (e->type && !type_equals(e->type, r)) {
        typecheck_error("types must match for assignment");
        e->type = nullptr;
      }
      break;

    case EXPR_IDX:
      e->type = expr_typecheck(e->left);
      if (e->type) {
        if (e->type->kind != TYPE_ARRAY && e->type->kind != TYPE_CARRAY) {
          typecheck_error("indexing requires an array");
          e->type = nullptr;
          // No break (still need to typecheck index)
        } else e->type = e->type->subtype;
      }

      struct type *idx = expr_typecheck(e->right);
      if (!idx) {
        e->type = nullptr;
      } else if (idx->kind != TYPE_INTEGER) {
        typecheck_error("array index must be an integer");
        e->type = nullptr;
      }
      break;

    case EXPR_CALL: e->type = expr_typecheck_call(e); break;

    case EXPR_ARG:  case EXPR_INIT:
      typecheck_error("list is not valid in this context");
      break;

    case EXPR_INT:    e->type = type_primitive(TYPE_INTEGER); break;
    case EXPR_BOOL:   e->type = type_primitive(TYPE_BOOLEAN); break;
    case EXPR_CHAR:   e->type = type_primitive(TYPE_CHAR   ); break;
    case EXPR_STR:    e->type = type_primitive(TYPE_STRING ); break;
    case EXPR_DOUBLE: e->type = type_primitive(TYPE_DOUBLE ); break;

    case EXPR_NAME:   e->type = e->symbol->type; break;
  }
  return e->type;
}

void expr_typecheck_init(struct type *want, struct expr *val) {
  const bool is_array = want->kind == TYPE_ARRAY || want->kind == TYPE_CARRAY;

  if (val->kind != EXPR_INIT) {
    if (is_array) {
      typecheck_error("array must be initialized with `{...}`");
      return;
    }

    struct type *got = expr_typecheck(val);
    if (got && !type_equals(want, got)) {
      typecheck_error("array initialized with incorrect type");
    }

    return;
  }

  if ((!is_array)) {
    typecheck_error("non-array type cannot be initialized with `{...}`");
    return;
  }

  // int64_t since it is the type used by ints (which determine array sizes)
  int64_t count = 0;
  for (const struct expr *a = val->left; a; a = a->right) {
    expr_typecheck_init(want->subtype, a->left);
    count++;
  }

  const bool want_size_is_valid = want->size
                               && want->size->type
                               && want->size->type->kind == TYPE_INTEGER;
  if (want_size_is_valid) {
    struct expr_constant size;
    const bool size_is_known =
      expr_status(want->size, &size) == EXPR_STAT_CONST;

    if (size_is_known && size.integer != count) {
      typecheck_error(
        "array of size %" PRId64 " given %" PRId64 " initializers",
        size.integer, count
      );
    }
  }
}

static expr_status_t expr_status_error(const char *msg) {
  // WARNING: passing msg directly would interpret arbitrary format specifiers (bad and dangerous).
  typecheck_error("%s", msg);
  return EXPR_STAT_ERROR;
}

static expr_status_t expr_constant_integer(
  expr_t kind, int64_t left, int64_t right, int64_t *out
) {
  bool overflow = false;
  switch (kind) {
    case EXPR_ADD: overflow = __builtin_add_overflow(left, right, out); break;
    case EXPR_SUB: overflow = __builtin_sub_overflow(left, right, out); break;
    case EXPR_MUL: overflow = __builtin_mul_overflow(left, right, out); break;
    case EXPR_DIV: case EXPR_MOD:
      if (right == 0)
        return expr_status_error("division by zero in constant expression");

      if (left == INT64_MIN && right == -1)
        return expr_status_error("integer overflow in constant expression");

      *out = kind == EXPR_DIV ? left / right : left % right;
      break;

    case EXPR_POW:
      // Nonpositive exponents produce 1 (matches `integer_power()`)
      *out = 1;
      while (right > 0) {
        if ((right & 1) && __builtin_mul_overflow(*out, left, out)) {
          overflow = true;
          break;
        }
        right /= 2;
        if (right && __builtin_mul_overflow(left, left, &left)) {
          overflow = true;
          break;
        }
      }
      break;

    default: unreachable();
  }

  if (overflow)
    return expr_status_error("integer overflow in constant expression");

  return EXPR_STAT_CONST;
}

expr_status_t expr_status(const struct expr *e, struct expr_constant *out) {
  assert(e); assert(out);

  *out = (struct expr_constant){.kind = e->kind};

  switch (e->kind) {
    case EXPR_INT:
      out->integer = e->int_literal;
      return EXPR_STAT_CONST;

    case EXPR_DOUBLE:
      out->real = e->double_literal;
      return EXPR_STAT_CONST;

    case EXPR_BOOL: case EXPR_CHAR: case EXPR_STR:
      return EXPR_STAT_CONST;

    case EXPR_INIT:
      for (const struct expr *a = e->left; a; a = a->right) {
        struct expr_constant element;
        expr_status_t status = expr_status(a->left, &element);
        if (status != EXPR_STAT_CONST) return status;
      }
      return EXPR_STAT_CONST;

    case EXPR_NEG: {
      struct expr_constant operand;
      expr_status_t status = expr_status(e->left, &operand);
      if (status != EXPR_STAT_CONST) return status;

      out->kind = operand.kind;
      if (operand.kind == EXPR_INT) {
        if (__builtin_sub_overflow((int64_t)0, operand.integer, &out->integer)) {
          return expr_status_error(
            "integer overflow in constant expression"
          );
        }
      } else {
        assert(operand.kind == EXPR_DOUBLE);
        out->real = -operand.real;
      }
      return EXPR_STAT_CONST;
    }

    case EXPR_ADD: case EXPR_SUB: case EXPR_MUL:
    case EXPR_DIV: case EXPR_MOD: case EXPR_POW: break;

    default: return EXPR_STAT_RUNTIME;
  }

  struct expr_constant left, right;
  expr_status_t left_status = expr_status(e->left, &left);
  expr_status_t right_status = expr_status(e->right, &right);

  if (left_status == EXPR_STAT_ERROR || right_status == EXPR_STAT_ERROR)
    return EXPR_STAT_ERROR;

  if (left_status == EXPR_STAT_RUNTIME || right_status == EXPR_STAT_RUNTIME)
    return EXPR_STAT_RUNTIME;

  assert(left.kind == right.kind);
  out->kind = left.kind;
  if (left.kind == EXPR_INT) {
    return expr_constant_integer(
      e->kind, left.integer, right.integer, &out->integer
    );
  }

  assert(left.kind == EXPR_DOUBLE);
  switch (e->kind) {
    case EXPR_ADD: out->real = left.real + right.real; break;
    case EXPR_SUB: out->real = left.real - right.real; break;
    case EXPR_MUL: out->real = left.real * right.real; break;
    case EXPR_DIV:
      if (right.real == 0.0) {
        return expr_status_error("division by zero in constant expression");
      }
      out->real = left.real / right.real;
      break;

    default: unreachable();  // Doubles not supported for MOD/POW
  }

  if (!isfinite(out->real))
    return expr_status_error("non-finite result in constant expression");

  return EXPR_STAT_CONST;
}

bool expr_validate(const struct expr *e) {
  if (!e) return true;
  if (e->kind == EXPR_NAME && !e->symbol) return false;
  if (e->kind != EXPR_ARG && e->kind != EXPR_INIT && !type_validate(e->type))
    return false;

  return expr_validate(e->left) && expr_validate(e->right);
}

int expr_lower_var(const struct symbol *s, const struct ir_func *func) {
  // Implementation goes here
  return 0;  // Placeholder
}

int expr_lower(struct expr *e, struct ir_func *func) {
  // Implementation goes here
  return 0;  // Placeholder
}
