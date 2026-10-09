#pragma once  /* "parser.h" is already used by Bison */

struct decl;

extern struct decl *parser_result;

unsigned int parser_error_count(void);
const char *token_name(int token);
