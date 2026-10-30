#include "resolver.h"
#include <stdarg.h>
#include <stdio.h>

static unsigned int err_count = 0;

bool resolver_trace = false;

unsigned int resolver_error_count(void) {
  return err_count;
}

void resolver_error(const char *fmt, ...) {
  err_count++;
  fprintf(stderr, "resolver error: ");
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}
