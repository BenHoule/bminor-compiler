#pragma once

#include <stdint.h>

/* Generated programs link this ABI, never the compiler executable. */
void print_integer(int64_t value);
void print_string(const char *value);
void print_boolean(int value);
void print_character(char value);
void print_double(double value);
int64_t integer_power(int64_t base, int64_t exponent);
