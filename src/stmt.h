
#pragma once

struct decl;
struct expr;

typedef enum {
  STMT_DECL,
  STMT_EXPR,
  STMT_IF_ELSE,
  STMT_FOR,
  STMT_PRINT,
  STMT_RETURN,
  STMT_BLOCK
} stmt_t;

struct stmt {
  stmt_t kind;

  union {
    // Simple statements (declaration, expression, block)
    struct decl *decl;
    struct stmt *block;
    struct expr *expr;

    // Control flow statements (if, while, etc.)
    struct {
      struct expr *cond_expr;
      struct stmt *body;
      union {
        // if (cond_expr) { body } else { else_body }
        struct stmt *else_body;

        // for (init_expr; cond_expr; loop_expr)
        struct {
          struct expr *init_expr;
          struct expr *loop_expr;
        };
      };
    };
  };

  struct stmt *next;           // Next statement in the sequence
};

// struct stmt * stmt_create( stmt_t kind, struct decl *decl, struct expr *init_expr, struct expr *expr, struct expr *next_expr, struct stmt *body, struct stmt *else_body, struct stmt *next );
struct stmt * stmt_create_simple(stmt_t kind, struct expr *expr, struct stmt *next);
struct stmt * stmt_create_print( struct expr *expr );
struct stmt * stmt_create_return( struct expr *expr );

struct stmt * stmt_create_decl(struct decl *decl, struct stmt *next);

struct stmt * stmt_create_block( struct stmt *block );
struct stmt * stmt_create_if_else( struct expr *cond_expr, struct stmt *body, struct stmt *else_body );
struct stmt * stmt_create_for( struct expr *init_expr, struct expr *cond_expr, struct expr *loop_expr, struct stmt *body );
void stmt_print( struct stmt *s, int indent );
