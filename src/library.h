
/* Generated programs link this ABI, as does the interpreter (`-interp`) */
#pragma once

#include <stdint.h>

/* Printing primitives */
void print_integer(int64_t value);
void print_string(const char *value);
void print_boolean(bool value);
void print_character(char value);
void print_double(double value);

/**
 * @brief Computes the power of an integer.
 *
 * @param base The base integer.
 * @param exponent The exponent integer.
 *
 * @return The result of `base ^ exponent`. Exits with a runtime error if overflow occurs.
 */
int64_t integer_power(int64_t base, int64_t exponent);

/**
 * @brief Allocates a zeroed array whose element 0 holds the length.
 *
 * @param dims An array of integers specifying the size of each dimension.
 * @param ndims The number of dimensions.
 *
 * @return A pointer to the newly allocated array.
 * Exits with a runtime error if allocation fails or any length is not positive.
 *
 * @note Nested arrays are allocated recursively: `dims[0]` is the outermost length.
 */
int64_t *bminor_array_new(const int64_t *dims, int64_t ndims);

/**
 * @brief Runtime division-by-zero trap.
 *
 * idivq raises SIGFPE on division by zero, this just translates that to a more
 * descriptive error message and terminates the program.
 */
[[noreturn]] void bminor_division_by_zero(void);

/**
 * @brief Runtime array out-of-bounds trap.
 *
 * Exits the program with a descriptive error message when an array index is out of bounds.
 */
[[noreturn]] void bminor_bounds_error(void);
