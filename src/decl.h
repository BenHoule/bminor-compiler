#pragma once

// Forward declarations
struct type;
struct expr;
struct stmt;
struct symbol;

/**
 * @brief Represents a declaration node in the abstract syntax tree.
 * @note See @ref decl_create parameters for field descriptions
 */
struct decl {
  char *name;
  struct type *type;
  struct expr *value;
  struct stmt *code;
  struct symbol *symbol;
  struct decl *next;
};

/**
 * @brief Creates new declaration node on the heap (with the given properties).
 *
 * @param[in] name  : Name of the declaration.
 * @param[in] type  : Type of the declaration.
 * @param[in] value : Associated initializer expression (variables).
 * @param[in] code  : Associated code block (functions).
 * @param[in] next  : Next declaration in the list.
 *
 * @return Pointer to the newly created declaration node.
 */
struct decl *decl_create(char *const name, struct type *const type, struct expr *const value,
                          struct stmt *const code, struct decl *const next);

/**
 * @brief Prints the given declaration with the specified indentation level.
 *
 * @param[in] d Pointer to the declaration to print.
 * @param[in] indent The indentation level for pretty-printing.
 */
void decl_print(const struct decl *d, int indent);

/**
 * @brief Resolves names and binds symbols for the given declaration list.
 * Performs scope-checking + ensures that declarations are correctly registered.
 *
 * @param[in,out] d Pointer to the declaration list to resolve.
 *
 * @note Early returns in `decl_resolve_func` (static helper) prevent bodies
 * of rejected function definitions from being resolved; Errors in a rejected
 * body are unreported.
 *
 * @warning Modifies AST, scopes/symbol tables, and `local_count` (static var).
 */
void decl_resolve( struct decl *d );

/**
 * @brief Typechecks the given declaration list and reports type errors.
 *
 * @param[in,out] d Pointer to the declaration list to typecheck.
 *
 * @note Modifies AST.
 */
void decl_typecheck( struct decl *d );
