#include "stmt.h"
#include "decl.h"
#include "expr.h"

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

  switch (s->kind) {
    case STMT_DECL:
      break;
    case STMT_EXPR:
      break;
    case STMT_IF_ELSE:
      fprintf(stderr, "If (");
      expr_print(s->cond_expr);
      fprintf(stderr, ") {\n");
      stmt_print(s->body, indent + 1);
      fprintf(stderr, "}");
      if (s->else_body) {
        fprintf(stderr, " else {\n");
        stmt_print(s->else_body, indent + 1);
        fprintf(stderr, "}");
      }
      break;
    case STMT_FOR:
      fprintf(stderr, "For (");
      expr_print(s->init_expr);
      fprintf(stderr, "; ");
      expr_print(s->cond_expr);
      fprintf(stderr, "; ");
      expr_print(s->loop_expr);
      fprintf(stderr, ") {\n");
      stmt_print(s->body, indent + 1);
      fprintf(stderr, "}");
      break;
    case STMT_PRINT:
      fprintf(stderr, "Print ");
      expr_print(s->expr);
      fprintf(stderr, "\n");
      break;
    case STMT_RETURN:
      fprintf(stderr, "Return ");
      expr_print(s->expr);
      fprintf(stderr, "\n");
      break;
    case STMT_BLOCK:
      fprintf(stderr, "{\n");
      stmt_print(s->block, indent + 1);
      fprintf(stderr, "}\n");
      break;
  }
  (void)s;
  (void)indent;
  return;
}
