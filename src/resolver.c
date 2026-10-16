#include "resolver.h"
#include <stdarg.h>
#include <stdio.h>

static unsigned int err_count = 0;

unsigned int resolver_error_count() {
  return err_count;
}

void resolver_error(const char *fmt, ...) {
  va_list args;
  err_count++;

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}
