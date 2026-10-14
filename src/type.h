#pragma once

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

struct param_list;
struct expr;

struct type {
	type_t kind;
	struct param_list *params;
	struct type *subtype;
  struct expr *size;
};

struct type * type_create( type_t kind, struct type *subtype, struct param_list *params, struct expr *size );

struct type * type_create_void();
struct type * type_create_boolean();
struct type * type_create_char();
struct type * type_create_integer();
struct type * type_create_double();
struct type * type_create_string();
struct type * type_create_array(struct type *subtype, struct expr *size);
struct type * type_create_carray(struct type *subtype, struct expr *size);
struct type * type_create_function(struct type *subtype, struct param_list *params);
struct type * type_create_auto();

void type_print( struct type *t );
