#pragma once

#include <stdio.h>

typedef enum {
  TOKEN_IDENTIFIER = 256,  /* 0-255 reserved for single-char ASCII tokens */

  /* ATOMIC TYPE TOKENS */
  TOKEN_BOOLEAN,
  TOKEN_CHAR,
  TOKEN_DOUBLE,
  TOKEN_FLOAT,  /* Reserved for expansion(?) */
  TOKEN_INTEGER,
  TOKEN_STRING,

  /* NON-ATOMIC TYPE TOKENS */
  TOKEN_ARRAY,
  TOKEN_AUTO,
  TOKEN_CARRAY,
  TOKEN_FUNCTION,
  TOKEN_VOID,

  /* LITERAL TOKENS */
  TOKEN_CHAR_LITERAL,
  TOKEN_DOUBLE_LITERAL,
  TOKEN_INTEGER_LITERAL,
  TOKEN_STRING_LITERAL,
  TOKEN_FALSE,
  TOKEN_TRUE,

  /* CONTROL TOKENS */
  TOKEN_ELSE,
  TOKEN_FOR,
  TOKEN_IF,
  TOKEN_RETURN,
  TOKEN_WHILE, /* Reserved for expansion */

  /* OPERATOR TOKENS (ops that use 2 chars) */
  TOKEN_INC,
  TOKEN_DEC,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_EQ,
  TOKEN_GE,
  TOKEN_LE,
  TOKEN_NE,

  /* PRINT (is a statement, not a function call) */
  TOKEN_PRINT

} token_t;

void scanner_open(FILE *);

int scanner_next_token(void);

void get_token_name(char *buf, int token, size_t buf_size);

unsigned int scanner_error_count(void);
