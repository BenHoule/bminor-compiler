#include "library.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc == 2 && strcmp(argv[1], "overflow") == 0) {
    integer_power(2, 63);
    return 2;
  }
  if (integer_power(2, 10) != 1024 ||
      integer_power(0, 0) != 1 ||
      integer_power(2, -1) != 1 ||
      integer_power(INT64_MAX, 1) != INT64_MAX ||
      integer_power(INT64_MIN, 1) != INT64_MIN ||
      integer_power(-2, 63) != INT64_MIN) {
    fprintf(stderr, "integer power regression\n");
    return 1;
  }
  print_integer(INT64_MAX);
  print_character(' ');
  print_boolean(1);
  print_character(' ');
  print_double(1.5);
  print_string("\n");
  return 0;
}
