#pragma once  /* "parser.h" is already used by Bison */

unsigned int parser_error_count(void);
const char *token_name(int token);
