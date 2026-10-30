#pragma once

// Forward declarations
struct type;
struct symbol;

/**
 * @brief Represents a parameter list node in the abstract syntax tree.
 */
struct param_list {
  char *name;
  struct type *type;
  struct symbol *symbol;
  struct param_list *next;
};

/**
 * @brief Creates a new parameter list node on the heap.
 *
 * @param[in] name The name of the parameter.
 * @param[in] type The type of the parameter.
 * @param[in] next Pointer to the next parameter in the list (can be nullptr).
 *
 * @return A pointer to the newly created parameter list node.
 *
 * @warning Compilation-owned; takes ownership of malloc-allocated name.
 */
struct param_list *param_list_create(char *name, struct type *type,
                                      struct param_list *next);

/**
 * @brief Prints the given parameter list.
 *
 * @param[in] a Pointer to the parameter list to print.
 */
void param_list_print(struct param_list *a);

/**
 * @brief Resolves names and binds symbols for the given parameter list.
 *
 * @param[in,out] a Pointer to the parameter list to resolve.
 *
 * @note Modifies AST and symbol tables.
 */
void param_list_resolve(struct param_list *a);
