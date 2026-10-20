#include "expr.h"
#include "resolver.h"
#include "scope.h"
#include "string_util.h"

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
