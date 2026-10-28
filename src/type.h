#pragma once

// Forward declarations
struct param_list;
struct expr;

typedef enum {
  TYPE_VOID,
  TYPE_BOOLEAN,
  TYPE_CHAR,
  TYPE_INTEGER,
  TYPE_DOUBLE,
  TYPE_STRING,
  TYPE_ARRAY,
  TYPE_CARRAY,
  TYPE_FUNCTION,
  TYPE_AUTO
} type_t;

/**
 * @brief Represents a type node in the abstract syntax tree.
 */
struct type {
  type_t kind;
  struct param_list *params;
  struct type *subtype;
  struct expr *size;
};

/**
 * @brief Creates a new type node.
 *
 * @param[in] kind The kind of the type.
 * @param[in] subtype The subtype (for arrays and functions).
 * @param[in] params The parameter list (for functions).
 * @param[in] size The size expression (for arrays).
 *
 * @return Pointer to the newly created type node.
 *
 * @warning Compilation-owned; released by memory_release(), not individually.
 */
struct type *type_create( type_t kind, struct type *subtype, struct param_list *params, struct expr *size );

// Constructors (aka type_create wrappers)
struct type *type_create_void(void);
struct type *type_create_boolean(void);
struct type *type_create_char(void);
struct type *type_create_integer(void);
struct type *type_create_double(void);
struct type *type_create_string(void);
struct type *type_create_array(struct type *subtype, struct expr *size);
struct type *type_create_carray(struct type *subtype, struct expr *size);
struct type *type_create_function(struct type *subtype, struct param_list *params);
struct type *type_create_auto(void);

/**
 * @brief Checks if two types are equal.
 *
 * @param[in] a Pointer to the first type.
 * @param[in] b Pointer to the second type.
 *
 * @return true if the types are equal, false otherwise.
 */
bool type_equals(struct type *a, struct type *b);

/**
 * @brief Prints the given type.
 *
 * @param[in] t Pointer to the type.
 */
void type_print( struct type *t );

/**
 * @brief Resolves names in array-size expressions.
 *
 * @param[in] t Pointer to the type.
 */
void type_resolve(struct type *t);

/**
 * @brief Checks if a type is a primitive type.
 *
 * @param[in] t Pointer to the type.
 * @return true if the type is primitive, false otherwise.
 *
 */
bool type_is_primitive(const struct type *t);

/**
 * @brief Returns the primitive type corresponding to the given kind.
 *
 * @param[in] kind TYPE_VOID through TYPE_STRING.
 * @return Borrowed canonical type; callers must not modify or free it.
 *
 */
struct type *type_primitive(type_t kind);

/**
 * @brief Typechecks the given type (mostly used for arrays).
 *
 * @param[in] t Pointer to the type.
 */
void type_typecheck(struct type *t);
