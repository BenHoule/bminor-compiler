#pragma once

/**
 * @brief Allocates a new scope on the heap and makes it the current scope.
 */
void scope_enter(void);

/**
 * @brief Makes parent the current scope and frees the exited scope.
 */
void scope_exit(void);

/**
 * @return The current scope depth as an integer.
 */
int scope_depth(void);

struct symbol;  // Forward declaration

/**
 * @brief Binds a name to a symbol in the innermost scope.
 *
 * @param[in] name The name to bind.
 * @param[in] symbol The symbol to associate with the name.
 *
 * @return 0 on success, non-zero on failure (e.g. redeclaration in same scope).
 */
int scope_bind(const char *name, struct symbol *symbol );

/**
 * @brief Searches innermost -> outermost for the given name.
 *
 * @param[in] name The name to look up.
 *
 * @return Symbol associated with the name if found, NULL otherwise.
 */
struct symbol *scope_lookup( const char *name );

/**
 * @brief Searches (only) innermost scope for the given name.
 *
 * @param[in] name The name to look up.
 *
 * @return Symbol associated with the name if found, NULL otherwise.
 */
struct symbol *scope_lookup_local( const char *name );
