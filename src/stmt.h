#pragma once

typedef enum {
  STMT_DECL,
  STMT_EXPR,
  STMT_IF_ELSE,
  STMT_FOR,
  STMT_PRINT,
  STMT_RETURN,
  STMT_BLOCK
} stmt_t;

// Forward declarations
struct decl;
struct expr;

/**
 * @brief Represents a statement node in the abstract syntax tree.
 */
struct stmt {
  stmt_t kind;
  struct stmt *next;    // Next statement in the sequence

  union {
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
};

// DEPRECATED: From dthain starter code - kept for posterity
// struct stmt *stmt_create(stmt_t kind, struct decl *decl, struct expr *init_expr, struct expr *expr, struct expr *next_expr, struct stmt *body, struct stmt *else_body, struct stmt *next);

/**
 * @brief Creates a statement node of the given kind on the heap.
 *
 * @param[in] kind The kind of statement.
 * @param[in] expr The expression associated with the statement (if any).
 *
 * @return Pointer to the newly created statement.
 *
 * @warning Compilation-owned; released by memory_release().
 */
struct stmt *stmt_create_simple(stmt_t kind, struct expr *expr);
struct stmt *stmt_create_expr(struct expr *expr);
struct stmt *stmt_create_print(struct expr *expr);
struct stmt *stmt_create_return(struct expr *expr);

/**
 * @brief Creates a declaration statement node on the heap.
 *
 * @param[in] decl The declaration associated with the statement.
 *
 * @return Pointer to the newly created statement.
 *
 * @warning Compilation-owned; released by memory_release().
 */
struct stmt *stmt_create_decl(struct decl *decl);

/**
 * @brief Creates a block statement node on the heap.
 *
 * @param[in] body  The body of the block statement.
 *
 * @return Pointer to the newly created statement.
 *
 * @warning Compilation-owned; released by memory_release().
 */
struct stmt *stmt_create_block(struct stmt *body);

/**
 * @brief Creates an if-else statement node on the heap.
 *
 * @param[in] cond_expr The condition expression.
 * @param[in] body The body of the if statement.
 * @param[in] else_body The body of the else statement (can be nullptr).
 *
 * @return Pointer to the newly created statement.
 *
 * @warning Compilation-owned; released by memory_release().
 */
struct stmt *stmt_create_if_else(struct expr *cond_expr, struct stmt *body, struct stmt *else_body);

/**
 * @brief Creates a for statement node on the heap.
 *
 * @param[in] init_expr The initialization expression.
 * @param[in] cond_expr The condition expression.
 * @param[in] loop_expr The loop expression.
 * @param[in] body The body of the for loop.
 *
 * @return Pointer to the newly created statement.
 *
 * @warning Compilation-owned; released by memory_release().
 */
struct stmt *stmt_create_for(struct expr *init_expr, struct expr *cond_expr, struct expr *loop_expr, struct stmt *body);

/**
 * @brief Prints the given statement with the specified indentation.
 *
 * @param[in] s Pointer to the statement to print.
 * @param[in] indent The indentation level.
 */
void stmt_print(struct stmt *s, int indent);

/**
 * @brief Resolves names and binds symbols for the given statement.
 *
 * @param[in,out] s Pointer to the statement to resolve.
 *
 * @note Modifies AST and symbol tables.
 */
void stmt_resolve(struct stmt *s);


struct type;  // Forward declaration

/**
 * @brief Typechecks the given statement against the specified type.
 *
 * @param[in] s Pointer to the statement to typecheck.
 * @param[in] ftype The expected type for return statements.
 */
void stmt_typecheck(struct stmt *s, struct type *ftype);

/**
 * @brief Read-only successful-frontend invariant check for statements.
 * @details Traverses the statement sequence and validates the data owned by
 * each statement kind.
 *
 * @param[in] s Pointer to the statement to validate.
 *
 * @return true if the statement is valid, false otherwise.
 */
bool stmt_validate(const struct stmt *s);

struct ir_func;  // Forward declaration

/**
 * @brief Lowers the given statement into the intermediate representation.
 *
 * @param[in] s Pointer to the statement to lower.
 * @param[in] func Pointer to the IR function context.
 *
 * @warning Modifies the IR function.
 */
void stmt_lower(struct stmt *s, struct ir_func *func);
