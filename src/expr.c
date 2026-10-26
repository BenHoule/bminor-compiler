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
  e->type = NULL;  // Initialize type to NULL
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
    case EXPR_NAME:   printf("%s", e->identifier.name  ); break;
    case EXPR_INT:    printf("%" PRId64, e->int_literal); break;
    case EXPR_DOUBLE: printf("%#.17g", e->double_literal); break;
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
    return e->kind != EXPR_NAME ||
      (e->identifier.symbol = scope_lookup(e->identifier.name));
}

// Morris Preorder Traversal - (Root-Left-Right) - Time: O(N), Space: O(1)
// NOTE: Messing with AST like this may cause race conditions
// TODO: Implement stack-based traversal (thread-safe)
void expr_resolve( struct expr *e ) {
  while (e) {
    if (!e->left) {
      if (!symbol_find_and_bind(e)) {
        //TODO: Add line/col to error message
        resolver_error("Undefined identifier '%s'", e->identifier.name);
      }
      e = e->right;
      continue;
    }

    // Find Inorder predecessor of current node (rightmost node in left subtree)
    struct expr *pre = e->left;
    while (pre->right && pre->right != e) {
      pre = pre->right;
    }

    if (pre->right != NULL) {  // Implies pre->right == e
      pre->right = NULL;       // Unlink (finished traversing left subtree)
      e = e->right;
      continue;                // Avoid double reporting for the same node
    }

    if (!symbol_find_and_bind(e)) {
      //TODO: Add line/col to error message
      resolver_error("Undefined identifier '%s'", e->identifier.name);
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
  return e->kind == EXPR_IDX ||
        (e->kind == EXPR_NAME && e->identifier.symbol &&
         e->identifier.symbol->type->kind != TYPE_FUNCTION &&
         e->identifier.symbol->type->kind != TYPE_VOID);  // This is gross: fix later
}

static struct type * expr_typecheck_binary(struct expr *e, bool allow_non_numeric) {
  if (!e) return NULL;

  struct type *l = expr_typecheck(e->left);
  struct type *r = expr_typecheck(e->right);
  if (!l || !r) return NULL;
  if (!type_equals(l, r)) {
    typecheck_error("'%s' requires matching operand types "
                    "(no implicit conversion)",
                    expr_op_name(e->kind));
    return NULL;
  }

  switch (l->kind) {
    case TYPE_INTEGER: break;
    case TYPE_DOUBLE:
      if (e->kind == EXPR_MOD || e->kind == EXPR_POW) {  // Matches C/C++
        typecheck_error("%s cannot operate on double operands",
                        expr_op_name(e->kind));
        return NULL;
      }  // NOTE: Could check type on code generation & use fmod()/pow() for doubles
      break;

    case TYPE_VOID: case TYPE_ARRAY: case TYPE_CARRAY: case TYPE_FUNCTION:
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

  e->type = expr_typecheck(e->left);
  struct type *r = expr_typecheck(e->right);
  if (!e->type || !r) return NULL;  // Avoid double-reporting
  if (e->type->kind != TYPE_BOOLEAN || r->kind != TYPE_BOOLEAN) {
    typecheck_error("logical operator '%s' requires boolean operands",
                    expr_op_name(e->kind));
    return NULL;
  }
  return type_primitive(TYPE_BOOLEAN);
}

static struct type * expr_typecheck_call(struct expr *e) {
  if (!e) return NULL;

  struct type *t = expr_typecheck(e->left);
  if (!t) return NULL;  // Avoid double-reporting
  if (t->kind != TYPE_FUNCTION) {
    typecheck_error("call requires a function");
    return t ? t->subtype : NULL;
  }

  int index = 1;
  const struct param_list *param = t->params;

  for (struct expr *arg = e->right; arg; arg = arg->right, index++) {
    if (!param) {
      typecheck_error("too many arguments in function call");
      return t->subtype;
    }

    struct type *arg_type = expr_typecheck(arg->left);
    if (arg_type && !type_equals(arg_type, param->type))
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
        e->type = NULL;
      }
      break;

    case EXPR_NEG:
      e->type = expr_typecheck(e->left);
      if (e->type && e->type->kind != TYPE_INTEGER &&
          e->type->kind != TYPE_DOUBLE) {
        typecheck_error("negation ('-') requires a numeric operand");
        e->type = NULL;
      }
      break;

    case EXPR_INC:  case EXPR_DEC:
      e->type = expr_typecheck(e->left);
      if (!e->type) break;  // Avoid double-reporting
      if (!expr_is_lvalue(e->left)) {
        typecheck_error("'%s' requires an lvalue",
                        expr_op_name(e->kind));
        e->type = NULL;
      } else if (e->type->kind != TYPE_INTEGER) {
        typecheck_error("'%s' requires an integer",
                        expr_op_name(e->kind));
        e->type = NULL;
      }
      break;

    case EXPR_LEN:
      e->type = expr_typecheck(e->left);
      if (!e->type) break;  // Avoid double-reporting
      if (e->type->kind != TYPE_ARRAY) {
        typecheck_error("length operator ('#') requires an array");
        e->type = NULL;
      } else e->type = type_primitive(TYPE_INTEGER);
      break;

    case EXPR_ASGN:
      e->type = expr_typecheck(e->left);
      if (e->type && !expr_is_lvalue(e->left)) {
        typecheck_error("left side of '=' is not assignable");
        e->type = NULL;
        // No break (still need to typecheck right-hand side)
      }

      struct type *r = expr_typecheck(e->right);
      if (!r) e->type = NULL;
      if (e->type && !type_equals(e->type, r)) {
        typecheck_error("types must match for assignment");
        e->type = NULL;
      }
      break;

    case EXPR_IDX:
      e->type = expr_typecheck(e->left);
      if (e->type) {
        if (e->type->kind != TYPE_ARRAY && e->type->kind != TYPE_CARRAY) {
          typecheck_error("indexing requires an array");
          e->type = NULL;
          // No break (still need to typecheck index)
        } else e->type = e->type->subtype;
      }

      struct type *idx = expr_typecheck(e->right);
      if (!idx) {
        e->type = NULL;
      } else if (idx->kind != TYPE_INTEGER) {
        typecheck_error("array index must be an integer");
        e->type = NULL;
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

    case EXPR_NAME:   e->type = e->identifier.symbol->type; break;
  }
  return e->type;
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

  // int64_t since it is the type used by ints (which determine array sizes)
  int64_t count = 0;
  for (const struct expr *a = val->left; a; a = a->right) {
    expr_typecheck_init(want->subtype, a->left);
    count++;
  }

  if (want->size && want->size->kind == EXPR_INT &&
      want->size->int_literal != count) {
    typecheck_error("array of size %" PRId64 " given %" PRId64 " initializers",
                    want->size->int_literal, count);
  }
}

bool expr_is_constant(const struct expr *e) {
  if (!e) return false;
  switch (e->kind) {
    case EXPR_INT: case EXPR_BOOL: case EXPR_CHAR: case EXPR_STR:
    case EXPR_DOUBLE:
      return true;

    case EXPR_NEG:
      return e->left->kind == EXPR_INT || e->left->kind == EXPR_DOUBLE;

    case EXPR_INIT:
      for (const struct expr *a = e->left; a; a = a->right)
        if (!expr_is_constant(a->left)) return false;

      return true;

    default: return false;
  }
}
