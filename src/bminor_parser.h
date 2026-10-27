#pragma once  // "parser.h" is already used by Bison

struct decl;  // Forward declaration

/**
 * @brief First node in the parsed AST.
 */
extern struct decl *parser_result;

/**
 * @return The number of recorded parser errors.
 */
unsigned int parser_error_count(void);

/**
 * @brief Returns the name of the given token.
 *
 * @param[in] token The token whose name is to be retrieved.
 * @return The name of the token as a string.
 */
const char *token_name(int token);
