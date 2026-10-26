#pragma once

// Forward declarations
struct type;
struct expr;
struct stmt;
struct symbol;

/**
 * @brief Represents a declaration in the abstract syntax tree.
 */
struct decl {
  char *name;               ///< Name of the identifier being declared
  struct type *type;        ///< Type of the identifier being declared
  struct expr *value;       ///< Associated initializer expression (variables)
  struct stmt *code;        ///< Associated code block statement (functions)
  struct symbol *symbol;    ///< Symbol table entry bound to `name`
  struct decl *next;        ///< Next declaration in the list
};

/**
 * @brief Creates a new declaration node on the heap (w/ the given properties).
 *
 * @param[in] name  : Name of the declaration.
 * @param[in] type  : Type of the declaration.
 * @param[in] value : Initializer associated with the declaration (variables).
 * @param[in] code  : Code block associated with the declaration (functions).
 * @param[in] next  : Next declaration in the list.
 * @return Pointer to the newly created declaration.
 */
struct decl * decl_create(char *name, struct type *type, struct expr *value,
                          struct stmt *code, struct decl *next);

/**
 * @brief Prints the given declaration with the specified indentation level.
 *
 * @param[in] d Pointer to the declaration to print.
 * @param indent The indentation level for pretty-printing.
 */
void decl_print(const struct decl *d, int indent);

/**
 * @brief Resolves names and binds symbols for the given declaration list.
 * Performs scope checking/ensures that declarations are correctly registered.
 *
 * @param[in,out] d Pointer to the declaration list to resolve.
 * @return void
 *
 * @note Edits `d` & its object(s), as well as scopes/symbol tables and static
 * var `local_count`.
 *
 * @warning Early returns in static helper `decl_resolve_func` prevent bodies
 * of rejected function definitions from being resolved; Errors in a rejected
 * body are unreported.
 */
void decl_resolve( struct decl *d );

/**
 * @brief Typechecks the given declaration list and reports type errors.
 *
 * @param[in,out] d Pointer to the declaration list to typecheck.
 * @return void
 *
 * @note Edits `d`.
 */
void decl_typecheck( struct decl *d );
