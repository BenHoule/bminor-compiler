#include "stmt.h"
#include "decl.h"
#include "expr.h"
#include "resolver.h"
#include "scope.h"
#include "type.h"
#include "typecheck.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// Create a new statement with the given properties.
struct stmt * stmt_create_simple(
  stmt_t kind,
  struct expr *expr,
  struct stmt *next
) {
  struct stmt *s = malloc(sizeof(struct stmt));
  if (!s) {
    fprintf(stderr, "Failed to allocate memory for statement\n");
    exit(EXIT_FAILURE);
  }
  s->kind = kind;
  s->expr = expr;
  s->next = next;
  return s;
}
struct stmt * stmt_create_expr( struct expr *expr ) {
  return stmt_create_simple(STMT_EXPR, expr, NULL);
}
struct stmt * stmt_create_return( struct expr *expr ) {
  return stmt_create_simple(STMT_RETURN, expr, NULL);
}
struct stmt * stmt_create_print( struct expr *expr ) {
  return stmt_create_simple(STMT_PRINT, expr, NULL);
}

struct stmt * stmt_create_decl( struct decl *decl, struct stmt *next ) {
  struct stmt *s = malloc(sizeof(struct stmt));
  if (!s) {
    fprintf(stderr, "Failed to allocate memory for statement\n");
    exit(EXIT_FAILURE);
  }
  s->kind = STMT_DECL;
  s->decl = decl;
  s->next = next;
  return s;
}

struct stmt  * stmt_create_block( struct stmt *block ) {
  struct stmt *s = malloc(sizeof(struct stmt));
  if (!s) {
    fprintf(stderr, "Failed to allocate memory for statement\n");
    exit(EXIT_FAILURE);
  }
  s->kind = STMT_BLOCK;
  s->block = block;
  s->next = NULL;
  return s;
}
struct stmt * stmt_create_if_else( struct expr *cond_expr, struct stmt *body, struct stmt *else_body ) {
  struct stmt *s = malloc(sizeof(struct stmt));
  if (!s) {
    fprintf(stderr, "Failed to allocate memory for statement\n");
    exit(EXIT_FAILURE);
  }
  s->kind = STMT_IF_ELSE;
  s->cond_expr = cond_expr;
  s->body = body;
  s->else_body = else_body;
  s->next = NULL;
  return s;
}
struct stmt * stmt_create_for( struct expr *init_expr, struct expr *cond_expr, struct expr *loop_expr, struct stmt *body ) {
  struct stmt *s = malloc(sizeof(struct stmt));
  if (!s) {
    fprintf(stderr, "Failed to allocate memory for statement\n");
    exit(EXIT_FAILURE);
  }
  s->kind = STMT_FOR;
  s->init_expr = init_expr;
  s->cond_expr = cond_expr;
  s->loop_expr = loop_expr;
  s->body = body;
  s->next = NULL;
  return s;
}

// Print a statement with the given indentation level.
void stmt_print( struct stmt *s, int indent ) {
  for (; s; s = s->next) {
    printf("%*s", indent * 2, "");

    switch (s->kind) {
      case STMT_DECL: decl_print(s->decl, indent); break;
      case STMT_EXPR: expr_print_affix("", s->expr, ";"); break;
      case STMT_IF_ELSE:
        printf("if (");
        expr_print(s->cond_expr);
        printf(") ");
        if (s->body->kind != STMT_BLOCK) stmt_print(s->body, indent);
        else stmt_print(s->body, indent);
        if (s->else_body) {
          printf(" else ");
          if (s->else_body->kind != STMT_BLOCK) stmt_print(s->else_body, indent);
          else stmt_print(s->else_body, indent);
        }
        break;
      case STMT_FOR:
        printf("for (");
        expr_print(s->init_expr);
        printf(";");
        if (s->cond_expr) {
          printf(" ");
          expr_print(s->cond_expr);
        }
        printf(";");
        if (s->loop_expr) {
          printf(" ");
          expr_print(s->loop_expr);
        }
        printf(") ");
        if (s->body->kind != STMT_BLOCK) stmt_print(s->body, indent);
        else stmt_print(s->body, indent);
        break;
      case STMT_PRINT:
        printf("print");
        if (s->expr) {
          printf(" ");
          expr_print(s->expr);
        }
        printf(";");
        break;
      case STMT_RETURN:
        printf("return");
        if (s->expr) {
          printf(" ");
          expr_print(s->expr);
        }
        printf(";");
        break;
      case STMT_BLOCK:
        indent++;
        printf("{\n%*s", indent * 2, "");
        stmt_print(s->block, indent);
        indent--;
        printf("\n%*s}", indent * 2, "");
        break;
      default: unreachable();
    }
    if (s->next) {
      printf("\n%*s", indent * 2, "");
    }
  }
}

void stmt_resolve( struct stmt *s ) {
  for(; s; s = s->next) {
    switch (s->kind) {
      case STMT_EXPR:
      case STMT_PRINT:
      case STMT_RETURN: expr_resolve(s->expr); break;
      case STMT_DECL: decl_resolve(s->decl); break;
      case STMT_BLOCK:
        scope_enter();
        stmt_resolve(s->block);
        scope_exit();
        break;
      case STMT_IF_ELSE:
        expr_resolve(s->cond_expr);
        stmt_resolve(s->body);
        stmt_resolve(s->else_body);
        break;
      case STMT_FOR:
        expr_resolve(s->init_expr);
        expr_resolve(s->cond_expr);
        expr_resolve(s->loop_expr);
        stmt_resolve(s->body);
        break;
      default: unreachable();  // Tells compiler to omit bounds check before jump table
    }
  }
}

static void stmt_expect_boolean(struct expr *e, const char *context) {
  struct type *t = expr_typecheck(e);
  if (t && t->kind != TYPE_BOOLEAN) {
    typecheck_error("'%s' requires a boolean expression", context);
  }
}

static void stmt_typecheck_return(struct stmt *s, struct type *ftype) {
  if (!s || !ftype) return;
  struct type *t = s->expr ? expr_typecheck(s->expr) : type_primitive(TYPE_VOID);
  if (!t) return;
  if (ftype->subtype->kind == TYPE_AUTO) {
    ftype->subtype = t;  // First return determines the type
    return;
  }
  if (!type_equals(t, ftype->subtype)) {
    typecheck_error("return type does not match function type");
  }
}

void stmt_typecheck( struct stmt *s, struct type *ftype ) {
  for (; s; s = s->next) {
    switch (s->kind) {
      case STMT_DECL: decl_typecheck(s->decl); break;
      case STMT_EXPR: expr_typecheck(s->expr); break;
      case STMT_BLOCK: stmt_typecheck(s->block, ftype); break;
      case STMT_RETURN: stmt_typecheck_return(s, ftype); break;
      case STMT_PRINT:
        for (struct expr *a = s->expr; a; a = a->right) {
          struct type *t = expr_typecheck(a->left);
          if (t && !type_is_primitive(t)) {
            typecheck_error("'print' requires primitive type");
          }
        }
        break;
      case STMT_IF_ELSE:
        stmt_expect_boolean(s->cond_expr, "if");
        stmt_typecheck(s->body, ftype);
        stmt_typecheck(s->else_body, ftype);
        break;
      case STMT_FOR:
        expr_typecheck(s->init_expr);
        if (s->cond_expr) stmt_expect_boolean(s->cond_expr, "for");
        expr_typecheck(s->loop_expr);
        stmt_typecheck(s->body, ftype);
        break;
      default: unreachable();
    }
  }
}
