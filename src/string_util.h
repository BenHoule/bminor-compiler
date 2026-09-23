#pragma once

#include <stdlib.h>

/* Decodes a quoted lexeme (including surrounding quotes) into raw bytes.
   Returns decoded length, or -1 if it exceeds out_size. */
int string_decode(const char *lexeme, char *out, size_t out_size);

/* Wraps raw bytes in quotes and re-escapes. Used by the printer. */
void string_encode(const char *raw, char *out, size_t out_size);
