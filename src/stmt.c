#include "stmt.h"
#include "decl.h"
#include "expr.h"
#include "ir.h"
#include "memory.h"
#include "scope.h"
#include "type.h"
#include "typecheck.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct stmt *stmt_create_simple(
  stmt_t kind,
  struct expr *expr
) {
  struct stmt *s = memory_allocate(sizeof *s);
  s->kind = kind;
  s->expr = expr;
  s->next = nullptr;
  return s;
}
struct stmt *stmt_create_expr(struct expr *expr) {
  return stmt_create_simple(STMT_EXPR, expr);
}
struct stmt *stmt_create_return(struct expr *expr) {
  return stmt_create_simple(STMT_RETURN, expr);
}
struct stmt *stmt_create_print(struct expr *expr) {
  return stmt_create_simple(STMT_PRINT, expr);
}
struct stmt *stmt_create_decl(struct decl *decl) {
  struct stmt *s = memory_allocate(sizeof *s);
  s->kind = STMT_DECL;
  s->decl = decl;
  s->next = nullptr;
  return s;
}
struct stmt *stmt_create_block(struct stmt *body) {
  struct stmt *s = memory_allocate(sizeof *s);
  s->kind = STMT_BLOCK;
  s->block = body;
  s->next = nullptr;
  return s;
}
struct stmt *stmt_create_if_else(
  struct expr *cond_expr, struct stmt *body, struct stmt *else_body
) {
  struct stmt *s = memory_allocate(sizeof *s);
  s->kind = STMT_IF_ELSE;
  s->cond_expr = cond_expr;
  s->body = body;
  s->else_body = else_body;
  s->next = nullptr;
  return s;
}
struct stmt *stmt_create_for(
  struct expr *init_expr, struct expr *cond_expr,
  struct expr *loop_expr, struct stmt *body
) {
  struct stmt *s = memory_allocate(sizeof *s);
  s->kind = STMT_FOR;
  s->init_expr = init_expr;
  s->cond_expr = cond_expr;
  s->loop_expr = loop_expr;
  s->body = body;
  s->next = nullptr;
  return s;
}

static const char *stmt_kind_name(stmt_t kind) {
  switch (kind) {
    case STMT_DECL:    return "DECL";
    case STMT_EXPR:    return "EXPR";
    case STMT_PRINT:   return "PRINT";
    case STMT_RETURN:  return "RETURN";
    case STMT_BLOCK:   return "BLOCK";
    case STMT_IF_ELSE: return "IF_ELSE";
    case STMT_FOR:     return "FOR";
    default: unreachable();
  }
}

void stmt_print(struct stmt *s, const int indent) {
  for (; s; s = s->next) {
    printf("%*s", indent * 2, "");

    switch (s->kind) {
      case STMT_DECL: decl_print(s->decl, indent); break;
      case STMT_EXPR: expr_print_affix("", s->expr, ";"); break;

      case STMT_IF_ELSE:
        printf("if (");
        expr_print(s->cond_expr);
        printf(") ");
        stmt_print(s->body, indent);
        if (s->else_body) {
          printf(" else ");
          stmt_print(s->else_body, indent);
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
        stmt_print(s->body, indent);
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
        printf("{\n%*s", (indent + 1) * 2, "");
        stmt_print(s->block, indent + 1);
        printf("\n%*s}", indent * 2, "");
        break;

      default: unreachable();
    }

    if (s->next) printf("\n%*s", indent * 2, "");
  }
}


void stmt_resolve(struct stmt *s) {
  for(; s; s = s->next) {
    switch (s->kind) {
      case STMT_EXPR:
      case STMT_PRINT:
      case STMT_RETURN:
        expr_resolve(s->expr);
        break;

      case STMT_DECL:
        decl_resolve(s->decl);
        break;

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
  if (!t) return;
  if (t->kind != TYPE_BOOLEAN)
    typecheck_error("'%s' requires a boolean expression", context);
}

static void stmt_typecheck_return(struct stmt *s, struct type *ftype) {
  if (!s || !ftype) return;

  struct type *t = s->expr
    ? expr_typecheck(s->expr)
    : type_primitive(TYPE_VOID);

  if (!t) return;  // Avoid double-reporting
  if (t->kind == TYPE_AUTO) {
    typecheck_error("cannot infer return type from unresolved type");
    return;
  }

  if (ftype->subtype->kind == TYPE_AUTO) {
    ftype->subtype = t;  // First return determines the type
    return;
  }

  if (!type_equals(t, ftype->subtype))
    typecheck_error("return type does not match function type");
}

void stmt_typecheck(struct stmt *s, struct type *ftype) {
  for (; s; s = s->next) {
    switch (s->kind) {
      case STMT_DECL:   decl_typecheck(s->decl); break;
      case STMT_EXPR:   expr_typecheck(s->expr); break;
      case STMT_BLOCK:  stmt_typecheck(s->block, ftype); break;
      case STMT_RETURN: stmt_typecheck_return(s, ftype); break;

      case STMT_PRINT:
        for (struct expr *a = s->expr; a; a = a->right) {
          struct type *t = expr_typecheck(a->left);
          if (t && !type_is_primitive(t))
            typecheck_error("'print' requires primitive type");
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


bool stmt_validate(const struct stmt *s) {
  for (; s; s = s->next) {
    switch (s->kind) {
      case STMT_DECL:
        if (!decl_validate(s->decl)) return false;
        break;
      case STMT_EXPR: case STMT_PRINT: case STMT_RETURN:
        if (!expr_validate(s->expr)) return false;
        break;
      case STMT_BLOCK:
        if (!stmt_validate(s->block)) return false;
        break;
      case STMT_IF_ELSE:
        if (!expr_validate(s->cond_expr) || !stmt_validate(s->body) ||
            !stmt_validate(s->else_body)) return false;
        break;
      case STMT_FOR:
        if (!expr_validate(s->init_expr) || !expr_validate(s->cond_expr) ||
            !expr_validate(s->loop_expr) || !stmt_validate(s->body)) return false;
        break;
      default: return false;
    }
  }
  return true;
}

void stmt_lower(struct stmt *s, struct ir_func *func) {
  for (; s; s = s->next) {
    switch (s->kind) {
      case STMT_DECL:  decl_lower(s->decl,  func); break;
      case STMT_EXPR:  expr_lower(s->expr,  func); break;
      case STMT_BLOCK: stmt_lower(s->block, func); break;

      case STMT_IF_ELSE:
        struct ir_block *true_block = ir_block_create(func);
        struct ir_block *join = ir_block_create(func);
        struct ir_block *false_block = s->else_body ? ir_block_create(func) : join;

        expr_lower_cond(s->cond_expr, func, true_block, false_block);

        ir_block_place(func, true_block);
        stmt_lower(s->body, func);
        ir_emit_jump(func, join);

        if (s->else_body) {
          ir_block_place(func, false_block);
          stmt_lower(s->else_body, func);
          ir_emit_jump(func, join);
        }

        ir_block_place(func, join);
        break;

      case STMT_FOR:
        struct ir_block *cond = ir_block_create(func);
        struct ir_block *body = ir_block_create(func);
        struct ir_block *step = ir_block_create(func);
        struct ir_block *done = ir_block_create(func);

        if (s->init_expr) expr_lower(s->init_expr, func);

        ir_block_place(func, cond);
        if (s->cond_expr) expr_lower_cond(s->cond_expr, func, body, done);

        ir_block_place(func, body);
        stmt_lower(s->body, func);

        ir_block_place(func, step);
        if (s->loop_expr) expr_lower(s->loop_expr, func);
        ir_emit_jump(func, cond);

        ir_block_place(func, done);
        break;

      case STMT_PRINT:
        for (struct expr *a = s->expr; a; a = a->right) {
          int value = expr_lower(a->left, func);
          ir_emit(func, IR_PRINT, IR_NONE, value, IR_NONE)->imm.kind = a->left->type->kind;
        }
        break;

      case STMT_RETURN: {
        int value = s->expr ? expr_lower(s->expr, func) : IR_NONE;
        ir_emit(func, IR_RETURN, IR_NONE, value, IR_NONE);
        break;
      }

      default:
        fprintf(stderr, "Lowering statement kind (%s) not supported yet\n", stmt_kind_name(s->kind));
        exit(EXIT_FAILURE);
    }
  }
}
