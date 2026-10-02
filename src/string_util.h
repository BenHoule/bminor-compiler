#pragma once

#include <stdlib.h>
#include <stdint.h>

constexpr size_t ERR_INVALID_CHAR = 0;
constexpr size_t ERR_TOO_LONG = SIZE_MAX;

// Encodes a single character literal into a raw byte.
// Returns the converted byte.
uint8_t char_encode(const unsigned char **in);

// Encodes a quoted string literal (including surrounding quotes) into raw bytes.
// Returns encoded length, SIZE_MAX if it exceeds out_size, or 0 if input is invalid.
size_t string_encode(const unsigned char *in, uint8_t *out, size_t out_size);

// Decodes raw bytes into a quoted string literal, re-escaping as necessary. Used by the printer.
size_t string_decode(const uint8_t *in, unsigned char *out, size_t out_size);
