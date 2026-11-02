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

void print_integer(const int64_t x) {
  printf("%" PRId64, x);
}

void print_string(const char *s) {
  printf("%s",s);
}

void print_boolean(const bool b) {
  printf("%s", b ? "true" : "false");
}

void print_character(const char c) {
  printf("%c",c);
}

void print_double(const double value) {
  printf("%.17g", value);
}

int64_t integer_power(int64_t x, int64_t y) {
  int64_t result = 1;
  while (y > 0) {
    if ((y & 1) && __builtin_mul_overflow(result, x, &result)) {
      fprintf(stderr, "bminor runtime: integer power overflow\n");
      exit(EXIT_FAILURE);
    }
    y /= 2;
    if (y && __builtin_mul_overflow(x, x, &x)) {
      fprintf(stderr, "bminor runtime: integer power overflow\n");
      exit(EXIT_FAILURE);
    }
  }
  return result;
}
