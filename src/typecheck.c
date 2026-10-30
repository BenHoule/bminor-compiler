#include "typecheck.h"

#include <stdarg.h>
#include <stdio.h>

static unsigned int error_count = 0;

unsigned int typecheck_error_count(void) {
  return error_count;
}

void typecheck_error(const char *fmt, ...) {
  //TODO: add line/col to error message
  error_count++;
  fprintf(stderr, "type error: ");
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}
