#pragma once

#include <stdio.h>

/**
 * @brief Opens the scanner with the given input file.
 *
 * @param[in] FILE* The input file to scan.
 */
void scanner_open(FILE *);

/**
 * @brief Retrieves the next token from the scanner.
 *
 * @return The next token as an integer.
 */
int scanner_next_token(void);

/**
 * @return The number of recorded scanner errors.
 */
unsigned int scanner_error_count(void);
