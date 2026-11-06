/*
This is the standard library of functions for bminor, implemented in C.
The print statement in bminor expects there to exist a function
for each type that can be printed.  So, the following bminor code:

x: integer = 10;
b: boolean = true;
s: string = "hello";

print x, b, s;

Is effectively translated to the following C code:

print_integer(x);
print_boolean(b);
print_string(s);

And the following bminor code:

x = a ^ b;

Is effectively this C code:

x = integer_power(a,b);
*/

#include "library.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

[[noreturn]] static void bminor_runtime_error(const char *message) {
  fprintf(stderr, "runtime error: %s\n", message);
  exit(EXIT_FAILURE);
}

void print_integer(const int64_t x)   { printf("%" PRId64, x); }
void print_string(const char *s)      { printf("%s", s); }
void print_boolean(const bool b)      { printf("%s", b ? "true" : "false"); }
void print_character(const char c)    { printf("%c", c); }
void print_double(const double value) { printf("%.17g", value); }

int64_t integer_power(int64_t x, int64_t y) {
  int64_t result = 1;
  while (y > 0) {
    if ((y & 1) && __builtin_mul_overflow(result, x, &result)) {
      bminor_runtime_error("integer power overflow");
    }
    y /= 2;
    if (y && __builtin_mul_overflow(x, x, &x)) {
      bminor_runtime_error("integer power overflow");
    }
  }
  return result;
}

int64_t *bminor_array_new(const int64_t *dims, int64_t ndims) {
  int64_t length = dims[0];
  if (length <= 0) bminor_runtime_error("array length must be positive");
  if ((uint64_t)length >= SIZE_MAX / sizeof(int64_t)) bminor_runtime_error("array length too large");

  int64_t *array = calloc((size_t)length + 1, sizeof(int64_t));
  if (!array) bminor_runtime_error("array allocation failed (out of memory)");
  array[0] = length;

  if (ndims > 1)
    for (int64_t i = 1; i <= length; i++)
      array[i] = (int64_t)(uintptr_t)bminor_array_new(dims + 1, ndims - 1);

  return array;
}


void bminor_division_by_zero(void) { bminor_runtime_error("division by zero"); }
void bminor_bounds_error(void)     { bminor_runtime_error("array index out of bounds"); }
