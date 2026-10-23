#include "expr.h"
#include "param_list.h"
#include "resolver.h"
#include "scope.h"
#include "string_util.h"
#include "symbol.h"
#include "type.h"
#include "typecheck.h"


#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h> // PRId64 and PRIu64 macros (for printing int64_t)
#include <string.h>

// Create a new expression node with the given kind and child expressions.
struct expr * expr_create( expr_t kind, struct expr *left, struct expr *right ) {
  struct expr *e = (struct expr *)malloc(sizeof(struct expr));
  if (!e) {
    fprintf(stderr, "Failed to allocate memory for expression\n");
    exit(EXIT_FAILURE);
  }
  e->kind = kind;
  e->left = left;
  e->right = right;
  e->int_literal = 0;
  return e;
}

struct expr * expr_create_name( const char *n ) {
  struct expr *e = expr_create( EXPR_NAME, NULL, NULL );
  e->identifier.name = n;
  e->identifier.symbol = NULL;
  return e;
}
struct expr * expr_create_integer_literal( int64_t i ) {
  struct expr *e = expr_create( EXPR_INT, NULL, NULL );
  e->int_literal = i;
  return e;
}
struct expr * expr_create_double_literal( double d ) {
  struct expr *e = expr_create( EXPR_DOUBLE, NULL, NULL );
  e->double_literal = d;
  return e;
}
struct expr * expr_create_boolean_literal( bool b ) {
  struct expr * e = expr_create( EXPR_BOOL, NULL, NULL );
  e->bool_literal = b;
  return e;
}
struct expr * expr_create_char_literal( char c ) {
  struct expr *e = expr_create( EXPR_CHAR, NULL, NULL );
  e->char_literal = c;
  return e;
}
struct expr * expr_create_string_literal( const char *str ) {
  struct expr *e = expr_create( EXPR_STR, NULL, NULL );
  e->string_literal = str;
  return e;
}
struct expr * expr_create_false() {
  struct expr *e = expr_create( EXPR_BOOL, NULL, NULL );
  e->bool_literal = false;
  return e;
}
struct expr * expr_create_true() {
  struct expr *e = expr_create( EXPR_BOOL, NULL, NULL );
  e->bool_literal = true;
  return e;
}

void expr_print_affix(const char* prefix, struct expr *e, const char* suffix) {
  if (prefix && *prefix != '\0') printf("%s", prefix);
  expr_print(e);
  if (suffix && *suffix != '\0') printf("%s", suffix);
}

static void expr_print_binop( struct expr *e, const char *op) {
  printf("(");
  expr_print(e->left);
  printf(" %s ", op);
  expr_print(e->right);
  printf(")");
}

// Print the expression `e` in a human-readable format
void expr_print( struct expr *e ) {
  if (!e) return;

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
    case EXPR_NEG: expr_print_affix("-", e->left, ""); break;
    case EXPR_LEN: expr_print_affix("#", e->left, ""); break;
    case EXPR_NOT: expr_print_affix("!", e->left, ""); break;
    case EXPR_INC: expr_print_affix("", e->left, "++"); break;
    case EXPR_DEC: expr_print_affix("", e->left, "--"); break;
    case EXPR_NAME:   printf("%s", e->identifier.name  ); break;
    case EXPR_INT:    printf("%" PRId64, e->int_literal); break;
    case EXPR_DOUBLE: printf("%.17g", e->double_literal); break;
    case EXPR_BOOL:   printf("%s", e->bool_literal ? "true" : "false"); break;
    case EXPR_CHAR:   printf("'%s'", char_decode(e->char_literal, '\'')); break;
    case EXPR_STR:
      static char decoded_str[1278] = "";  // 255(max string length) * 5 + 3
      string_decode(e->string_literal, decoded_str, sizeof(decoded_str));
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

// Returns True on success, False on failure
static bool symbol_find_and_bind(struct expr *e) {
    return e->kind == EXPR_NAME &&
      !(e->identifier.symbol = scope_lookup(e->identifier.name));
}

// Morris Preorder Traversal - (Root-Left-Right) - Time: O(N), Space: O(1)
// NOTE: Messing with AST like this may cause race conditions
// TODO: Implement stack-based traversal (thread-safe)
void expr_resolve( struct expr *e ) {
  while (e) {
    if (!e->left) {
      if (symbol_find_and_bind(e)) {
        //TODO: Add line/col to error message
        resolver_error("Error: Undefined symbol '%s'", e->identifier.name);
      }
      e = e->right;
      continue;
    }

    // Find Inorder predecessor of current node
    struct expr *pre = e->left;
    while (pre->right && pre->right != e) {
      pre = pre->right;
    }
    if (pre->right != NULL) {
      pre->right = NULL;  // Unlink (left subtree traversed)
      e = e->right;
      continue;
    }
    if (symbol_find_and_bind(e)) {
      //TODO: Add line/col to error message
      resolver_error("Error: Undefined symbol '%s'", e->identifier.name);
    }
    pre->right = e;  // Link (until we finish traversing left subtree)
    e = e->left;
  }
}

static const char * expr_op_name( expr_t kind ) {
  switch (kind) {
    case EXPR_ADD: return "+";   case EXPR_SUB: return "-";
    case EXPR_MUL: return "*";   case EXPR_DIV: return "/";
    case EXPR_MOD: return "%%";  case EXPR_POW: return "^";
    case EXPR_NEG: return "-";   case EXPR_NOT: return "!";
    case EXPR_INC: return "++";  case EXPR_DEC: return "--";
    case EXPR_LEN: return "#";
    case EXPR_LT:  return "<";   case EXPR_GT:  return ">";
    case EXPR_LE:  return "<=";  case EXPR_GE:  return ">=";
    case EXPR_EQ:  return "==";  case EXPR_NE:  return "!=";
    case EXPR_AND: return "&&";  case EXPR_OR:  return "||";
    case EXPR_ASGN: return "=";
    default: return "?";
  }
}

static bool expr_is_lvalue(const struct expr *e) {
  if (!e) return false;
  return e->kind == EXPR_IDX ||
         e->kind == EXPR_NAME && e->identifier.symbol &&
         e->identifier.symbol->type->kind != TYPE_FUNCTION;  // This is gross: fix later
}

static struct type * expr_typecheck_binary(struct expr *e, bool allow_non_numeric) {
  if (!e) return NULL;
  struct type *l = expr_typecheck(e->left);
  struct type *r = expr_typecheck(e->right);
  if (!l || !r) return NULL;
  if (!type_equals(l, r)) {
    typecheck_error("'%s' requires matching operand types"
                    "(no implicit conversion)",
                    expr_op_name(e->kind));
    return NULL;
  }
  switch (l->kind) {
    case TYPE_INTEGER: break;
    case TYPE_DOUBLE: if (e->kind == EXPR_MOD) {  // Matches C/C++ [requires fmod()]
        typecheck_error("%% cannot operate on double operands");
        return NULL;
      }  // NOTE: Could check type on code generation & use fmod() for doubles
      break;

    case TYPE_VOID:
    case TYPE_ARRAY:
    case TYPE_CARRAY:
    case TYPE_FUNCTION:
      typecheck_error("'%s' cannot operate on non-primitive types",
                      expr_op_name(e->kind));
      return NULL;

    default:
      if (allow_non_numeric) break;
      typecheck_error("'%s' requires numeric operands",
                      expr_op_name(e->kind));
      return NULL;
  }

  return (e->kind <= EXPR_POW) ? l : type_primitive(TYPE_BOOLEAN);
}

static struct type * expr_typecheck_logical(struct expr *e) {
  if (!e) return NULL;
  struct type *l = expr_typecheck(e->left);
  struct type *r = expr_typecheck(e->right);
  if (!l || !r) return NULL;
  if (l->kind != TYPE_BOOLEAN || r->kind != TYPE_BOOLEAN) {
    typecheck_error("logical operator '%s' requires boolean operands",
                    expr_op_name(e->kind));
    return NULL;
  }
  return type_primitive(TYPE_BOOLEAN);
}

static struct type * expr_typecheck_call(struct expr *e) {
  if (!e) return NULL;
  struct type *t = expr_typecheck(e->left);
  if (!t || t->kind != TYPE_FUNCTION) {
    typecheck_error("call requires a function");
    return t ? t->subtype : NULL;
  }
  const struct param_list *param = t->params;
  int index = 1;
  for (struct expr *arg = e->right; arg; arg = arg->right, index++) {
    if (!param) {
      typecheck_error("too many arguments in function call");
      return t->subtype;
    }

    struct type *arg_type = expr_typecheck(arg);
    if (!type_equals(arg_type, param->type))
      typecheck_error("argument %d has incorrect type", index);

    param = param->next;
  }
  if (param) typecheck_error("too few arguments in function call");
  return t->subtype;
}


// Returns NULL when `e` contains an error that has already been reported
struct type * expr_typecheck(struct expr *e) {
  if (!e) return NULL;
  switch (e->kind) {
    case EXPR_ADD:
    case EXPR_SUB:
    case EXPR_MUL:
    case EXPR_DIV:
    case EXPR_MOD:
    case EXPR_POW:
    case EXPR_LT:
    case EXPR_GT:
    case EXPR_LE:
    case EXPR_GE: return expr_typecheck_binary(e, false);

    case EXPR_EQ:
    case EXPR_NE: return expr_typecheck_binary(e, true);

    case EXPR_AND:
    case EXPR_OR: return expr_typecheck_logical(e);

    case EXPR_NOT: {
      struct type *t = expr_typecheck(e->left);
      if (t && t->kind != TYPE_BOOLEAN) {
        typecheck_error("logical NOT ('!') requires a boolean operand");
        return NULL;
      }
      return t;
    }
    case EXPR_NEG: {
      struct type *t = expr_typecheck(e->left);
      if (t && t->kind != TYPE_INTEGER && t->kind != TYPE_DOUBLE) {
        typecheck_error("negation ('-') requires an integer or double operand");
        return NULL;
      }
      return t;
    }
    case EXPR_INC:
    case EXPR_DEC: {
      struct type *t = expr_typecheck(e->left);
      if (!t) return NULL;
      if (!expr_is_lvalue(e->left)) {
        typecheck_error("'%s' requires an lvalue", expr_op_name(e->kind));
        return NULL;
      }
      if (t->kind != TYPE_INTEGER) {
        typecheck_error("'%s' requires an integer", expr_op_name(e->kind));
        return NULL;
      }
      return t;
    }
    case EXPR_LEN: {
      struct type *t = expr_typecheck(e->left);
      if (t && t->kind != TYPE_ARRAY) {
        typecheck_error("length operator ('#') requires an array");
        return NULL;
      }
      return t ? type_primitive(TYPE_INTEGER) : NULL;
    }
    case EXPR_ASGN: {
      struct type *l = expr_typecheck(e->left);
      struct type *r = expr_typecheck(e->right);
      if (!l || !r) return NULL;
      if (!expr_is_lvalue(e->left)) {
        typecheck_error("left side of '=' is not assignable");
        return NULL;
      }
      if (!type_is_primitive(l)) {
        typecheck_error("left side of '=' must be a primitive type");
        return NULL;
      }
      if (!type_equals(l, r)) {
        typecheck_error("types must match for assignment");
        return NULL;
      }
      return l;
    }
    case EXPR_IDX: {
      struct type *a = expr_typecheck(e->left);
      struct type *i = expr_typecheck(e->right);
      if (!a || !i) return NULL;
      if (a->kind != TYPE_ARRAY) {
        typecheck_error("indexing requires an array");
        return NULL;
      }
      if (i->kind != TYPE_INTEGER) {
        typecheck_error("array index must be an integer");
        return NULL;
      }
      return a->subtype;

    }
    case EXPR_CALL: return expr_typecheck_call(e);

    case EXPR_ARG:
    case EXPR_INIT:
      typecheck_error("list is not valid in this context");
      return NULL;

    case EXPR_INT:    return type_primitive(TYPE_INTEGER);
    case EXPR_BOOL:   return type_primitive(TYPE_BOOLEAN);
    case EXPR_CHAR:   return type_primitive(TYPE_CHAR);
    case EXPR_STR:    return type_primitive(TYPE_STRING);
    case EXPR_DOUBLE: return type_primitive(TYPE_DOUBLE);
    case EXPR_NAME:   return e->identifier.symbol->type;
  }
  unreachable();
}

void expr_typecheck_init(struct type *want, struct expr *val) {
  bool is_array = want->kind == TYPE_ARRAY || want->kind == TYPE_CARRAY;

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

  // uint64_t since it is the type used by ints (which determine array sizes)
  uint64_t count = 0;
  for (const struct expr *a = val->left; a; a = a->right) {
    expr_typecheck_init(want->subtype, a->left);
    count++;
  }
  if (want->size && want->size->kind == EXPR_INT &&
      want->size->int_literal != count) {
    typecheck_error("array of size %" PRIu64 " given %" PRIu64 " initializers",
                    want->size->int_literal, count);
  }
}

bool expr_is_constant(const struct expr *e) {
  if (!e) return false;
  switch (e->kind) {
    case EXPR_INT:
    case EXPR_BOOL:
    case EXPR_CHAR:
    case EXPR_STR:
    case EXPR_DOUBLE:
      return true;
    case EXPR_NEG:
      return e->left->kind == EXPR_INT || e->left->kind == EXPR_BOOL;
    case EXPR_INIT:
      for (const struct expr *a = e->left; a; a = a->right) {
        if (!expr_is_constant(a->left)) return false;
      }
      return true;
    default:
      return false;
  }
}
