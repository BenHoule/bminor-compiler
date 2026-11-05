#pragma once

#include <stdint.h>

/* Generated programs link this ABI, never the compiler executable. */
void print_integer(int64_t value);
void print_string(const char *value);
void print_boolean(bool value);
void print_character(char value);
void print_double(double value);
int64_t integer_power(int64_t base, int64_t exponent);

/**
 * @brief Handle division by zero in bminor programs.
 *
 * idivq raises SIGFPE on division by zero, this just translates that to a more
 * descriptive error message and terminates the program.
 */
[[noreturn]] void bminor_division_by_zero(void);
