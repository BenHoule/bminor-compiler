#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum {
	EXPR_ADD,     // 'e + e'
	EXPR_SUB,     // 'e - e'
	EXPR_MUL,     // 'e * e'
	EXPR_DIV,     // 'e / e'
  EXPR_MOD,     // 'e % e'
  EXPR_POW,     // 'e ^ e'
  EXPR_NEG,     // '-e'
  EXPR_INC,     // 'e++'
  EXPR_DEC,     // 'e--'
  EXPR_LEN,     // '#e'
  EXPR_LT,      // 'e < e'
  EXPR_GT,      // 'e > e'
  EXPR_LE,      // 'e <= e'
  EXPR_GE,      // 'e >= e'
  EXPR_EQ,      // 'e == e'
  EXPR_NE,      // 'e != e'
  EXPR_AND,     // 'e && e'
  EXPR_OR,      // 'e || e'
  EXPR_NOT,     // '!e'
  EXPR_ASGN,    // 'e = e'
  EXPR_IDX,     // 'e[e]'
  EXPR_CALL,    // 'e(...)'
  EXPR_ARG,     // '(e, ...)'
  EXPR_INIT,    // '{e, ...}'
  EXPR_INT,     // 'i'
  EXPR_BOOL,    // 'b'
  EXPR_CHAR,    // 'c'
  EXPR_STR,     // 'c1...ck'
  EXPR_DOUBLE,  // 'i.i'
  EXPR_NAME     // 'x'
} expr_t;

// Forward declarations
struct symbol;
struct type;

/**
 * @brief Represents an expression node in the abstract syntax tree.
 */
struct expr {
	// Used by all exprs
	expr_t kind;
	struct expr *left;
	struct expr *right;
  struct type *type;  // set by expr_typecheck()

  union {
    struct {  // Used by Identifiers
      const char *name;
      struct symbol *symbol;
    };

    bool bool_literal;  ///< Negligibly larger compiler >>> 64-bits for booleans
    char char_literal;
    double double_literal;
    int64_t int_literal;  ///< B-minor uses 64-bit integers
    const char *string_literal;
  };
};

/**
 * @brief Creates new expression node on the heap (with the given properties)
 *
 * @param[in] kind The kind of expression to create.
 * @param[in] left The left child expression (can be NULL).
 * @param[in] right The right child expression (can be NULL).
 *
 * @return A pointer to the newly created expression node.
 *
 * @warning Returned expression node must be freed by the caller.
 */
struct expr *expr_create(expr_t kind, struct expr *left, struct expr *right);

// Additional constructors (aka `expr_create` wrappers)
struct expr *expr_create_name(const char *n);
struct expr *expr_create_integer_literal(int64_t c);
struct expr *expr_create_double_literal(double c);
struct expr *expr_create_boolean_literal(bool c);
struct expr *expr_create_char_literal(char c);
struct expr *expr_create_string_literal(const char *str);
struct expr *expr_create_false(void);
struct expr *expr_create_true(void);

/**
 * @brief Prints the given expression.
 *
 * @param[in] e Pointer to the expression to print.
 */
void expr_print(struct expr *e);

/**
 * @brief Prints the given expression with a prefix and suffix.
 *
 * @param[in] prefix The prefix string to print before the expression.
 * @param[in] e Pointer to the expression to print.
 * @param[in] suffix The suffix string to print after the expression.
 */
void expr_print_affix(const char* prefix, struct expr *e, const char* suffix);

/**
 * @brief Resolves names and binds symbols for the given expression.
 *
 * @param[in,out] e Pointer to the expression to resolve.
 */
void expr_resolve(struct expr *e);

/**
 * @brief Typechecks the given expression and returns its type.
 *
 * @param[in,out] e Pointer to the expression to typecheck.
 *
 * @return The type of the expression.
 */
struct type *expr_typecheck(struct expr *e);

/**
 * @brief Typechecks the given initializer expression against the expected type.
 *
 * @param[in] want The expected type.
 * @param[in,out] val Pointer to the initializer expression to typecheck.
 */
void expr_typecheck_init(struct type *want, struct expr *val);

/**
 * @brief Checks if the given expression is a constant expression.
 *
 * @param[in] e Pointer to the expression to check.
 *
 * @return true if the expression is a literal or an initializer composed of
 * constant expressions, false otherwise.
 */
bool expr_is_constant(const struct expr *e);
