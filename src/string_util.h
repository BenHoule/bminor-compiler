#pragma once

#include <stdlib.h>
#include <stdint.h>

/* Returns true if `byte` is a printable ASCII character (32-126), false otherwise. */
static inline bool is_printable(const uint8_t byte);

/* Converts a raw byte into a hexadecimal string representation. */
static inline void byte_to_hexstr(unsigned char *out, const uint8_t raw);

/* Converts a hexadecimal character into its decimal value. */
static inline uint8_t hexval(unsigned char c);

/* Encodes a single character literal into a raw byte.
   Returns the converted byte. */
uint8_t char_encode(const unsigned char **in);

/* Encodes a quoted string literal (including surrounding quotes) into raw bytes.
   Returns encoded length, or 0 if it exceeds out_size. */
size_t string_encode(const unsigned char *in, uint8_t *out, size_t out_size);

/* Decodes raw bytes into a quoted string literal, re-escaping as necessary. Used by the printer. */
size_t string_decode(const uint8_t *in, unsigned char *out, size_t out_size);
