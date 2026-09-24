#ifndef TYPE_H
#define TYPE_H

struct param_list;
struct expr;

typedef enum {
	TYPE_VOID,
	TYPE_BOOLEAN,
	TYPE_CHARACTER,
	TYPE_INTEGER,
  TYPE_DOUBLE,
	TYPE_STRING,
	TYPE_ARRAY,
  TYPE_CARRAY,
	TYPE_FUNCTION,
  TYPE_AUTO
} type_t;

struct type {
	type_t kind;
	struct param_list *params;
	struct type *subtype;
  struct expr *size;
};

struct type * type_create( type_t kind, struct type *subtype, struct param_list *params, struct expr *size );
void          type_print( struct type *t );

#endif
