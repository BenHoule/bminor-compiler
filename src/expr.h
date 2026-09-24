#ifndef EXPR_H
#define EXPR_H

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
  EXPR_INT,     // 'i' (integer literal)
  EXPR_BOOL,    // 'b' (boolean literal)
  EXPR_CHAR,    // 'c' (character literal)
  EXPR_STR,     // 'c1...ck' (string literal)
  EXPR_DOUBLE,  // 'i.i' (double literal)
  EXPR_NAME     // 'x' (name)
} expr_t;

struct symbol;  // Forward declaration

struct expr {
	/* used by all kinds of exprs */
	expr_t kind;
	struct expr *left;
	struct expr *right;

  /* union minimizes memory footprint by overlapping mutually exclusive fields */
  union {
    /* used by call exprs (and operators with variable operands) */
    struct {
      struct expr **argv;
      size_t argc;
    } call;

    /* used by various leaf exprs */
    struct {
      const char *name;
      int64_t literal_value;  // B-minor uses 64-bit integers
      double double_literal;
      const char *string_literal;
      struct symbol *symbol;
    };
  };
};

struct expr * expr_create( expr_t kind, struct expr *left, struct expr *right );

struct expr * expr_create_name( const char *n );
struct expr * expr_create_integer_literal( int64_t c );
struct expr * expr_create_double_literal( double c );
struct expr * expr_create_boolean_literal( int c );
struct expr * expr_create_char_literal( char c );
struct expr * expr_create_string_literal( const char *str );

void expr_print( struct expr *e );

#endif
