#include "stmt.h"
#include "decl.h"
#include "expr.h"
#include "scope.h"

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
  if (!s) {
    printf("NULL_STMT");
    return;
  }

  switch (s->kind) {
    case STMT_DECL: decl_print(s->decl, indent); break;
    case STMT_EXPR:
      expr_print(s->expr);
      printf(";");
      break;
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
      if (s->init_expr) expr_print(s->init_expr);
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
  }
  if (s->next) {
    printf("\n%*s", indent * 2, "");
    stmt_print(s->next, indent);
  }
}

void stmt_resolve( struct stmt *s ) {
  while(s) {
    switch (s->kind) {
      case STMT_DECL:  decl_resolve(s->decl ); break;
      case STMT_BLOCK:
        scope_enter();
        stmt_resolve(s->block);
        scope_exit();
        break;
      case STMT_EXPR:
      case STMT_PRINT:
      case STMT_RETURN: expr_resolve(s->expr); break;
      case STMT_IF_ELSE:
        expr_resolve(s->cond_expr);
        assert(s->body);  // Guaranteed by the grammar

        // NOTE: Messing with the AST like this may cause race conditions
        if (s->body->next != s) {
          assert(s->body->next == NULL);  // Guaranteed by grammar
          s->body->next = s;
          s = s->body;
          continue;
        }
        if (s->else_body) { // NOT guaranteed by grammar
          if (s->else_body->next != s) {
            assert(s->else_body->next == NULL);  // Guaranteed by grammar
            s->else_body->next = s;
            s = s->else_body;
            continue;
          }
          s->else_body->next = NULL;
        }
        s->body->next = NULL;
        break;
      case STMT_FOR:
        expr_resolve(s->init_expr);
        expr_resolve(s->cond_expr);
        expr_resolve(s->loop_expr);

        // NOTE: Messing with the AST like this may cause race conditions
        if (s->body) {
          if (s->body->next == s) scope_exit();
          else {
            assert(s->body->next == NULL);  // Guaranteed by grammar
            s->body->next = s;
            s = s->body;
            scope_enter();
            continue;
          }
          s->body->next = NULL;
        }
        break;
      default: unreachable();  // Tells compiler to omit bounds check before jump table
    }
    s = s->next;
  }
}
