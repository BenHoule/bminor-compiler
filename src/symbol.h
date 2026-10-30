#pragma once

// Forward declarations
struct type;

typedef enum {
  SYMBOL_LOCAL,    // `which` = logical local index (not a byte offset)
  SYMBOL_PARAM,    // `which` = parameter index
  SYMBOL_GLOBAL    // `which` = unused
} symbol_t;

/**
 * @brief Metadata representing an identifier.
 */
struct symbol {
  symbol_t kind;
  struct type *type;
  char *name;
  int which;         // -1 for globals
  bool is_defined;   // Functions only
  int frame_locals;  // Functions only: local declaration count
};

/**
 * @brief Creates a new symbol on the heap.
 *
 * @param kind The kind of the symbol.
 * @param type The type of the symbol.
 * @param name The name of the symbol.
 *
 * @return Pointer to the newly created symbol.
 *
 * @warning Compilation-owned; borrows its name and type.
 */
struct symbol *symbol_create(symbol_t kind, struct type *type, char *name);

/**
 * @brief Prints the binding information of a symbol.
 *
 * @param s Pointer to the symbol.
 */
void symbol_print_binding(const struct symbol *s);
