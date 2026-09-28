#pragma once

#include <stdlib.h>

/* Encodes a single character literal (including surrounding quotes) into a raw byte.
   Returns true on success, false on failure. */
bool char_encode(const char *in, char *out);

/* Encodes a quoted string literal (including surrounding quotes) into raw bytes.
   Returns encoded length, or 0 if it exceeds out_size. */
size_t string_encode(const char *lexeme, char *out, size_t out_size);

/* Decodes raw bytes into a quoted string literal, re-escaping as necessary. Used by the printer. */
void string_decode(const char *raw, char *out, size_t out_size);
