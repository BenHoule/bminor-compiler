#include "string_util.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>

static inline bool is_printable(const uint8_t byte) {
  // spec says "between 32 and 127" but spaces need to be included
  // while DEL is not printable(?).
  enum { SPACE = 32, DEL = 127 };  // compile-time constants
  return SPACE <= byte && byte < DEL;
}

static inline void byte_to_hexstr(char **out, const uint8_t raw) {
  static const char hex_digits[] = "0123456789ABCDEF";
  **out = hex_digits[(raw >> 4) & 0xF];  // High nibble
  (*out)++;
  **out = hex_digits[raw & 0xF];  // Low nibble
}

int char_decode(const char *in, char *out, size_t out_size) {
  assert(in[0] == '\'');
  assert(out != NULL);  //TODO: make these only active during debugging

  in++;  // Skip the opening quote
  if (*in == '\\' && *(in + 1)) {
    in++;  // Skip the escape character
    switch (*in) {
      case 'a': *out = '\a'; break;
      case 'b': *out = '\b'; break;
      case 'e': *out = '\e'; break;
      case 'f': *out = '\f'; break;
      case 'n': *out = '\n'; break;
      case 'r': *out = '\r'; break;
      case 't': *out = '\t'; break;
      case 'v': *out = '\v'; break;
      case '\\': *out = '\\'; break;
      case '\'': *out = '\''; break;
      default: *out = *in; break;
    }
  } else {
    *out = *in;
  }

  out++;
  out_size--;
  in++;

  if (*in != '\'') {
    return 1;  // Unterminated char
  }

  return 0;
}

// Maps chars -> byte values
int string_decode(const char *in, char *out, size_t out_size) {
  assert(in[0] == '"' || in[0] == '\'');
  assert(out != NULL);  //TODO: make these only active during debugging

  in++;  // Skip the opening quote
  while (*in && *in != '"' && *in != '\'') {
    if (out_size == 0) {
      return 1;  // Buffer overflow
    }

    if (*in == '\\' && *(in + 1)) {
      in++;  // Skip the escape character
      switch (*in) {
        case 'a': *out = '\a'; break;
        case 'b': *out = '\b'; break;
        case 'e': *out = '\e'; break;
        case 'f': *out = '\f'; break;
        case 'n': *out = '\n'; break;
        case 'r': *out = '\r'; break;
        case 't': *out = '\t'; break;
        case 'v': *out = '\v'; break;
        case '\\': *out = '\\'; break;
        case '\'': *out = '\''; break;
        case '"': *out = '"'; break;
        case '0':
          if (in[1] == 'x') {
            char hex[3] = {0};
            if (isxdigit(in[2]) && isxdigit(in[3])) {
              hex[0] = in[2];
              hex[1] = in[3];
              *out = (char)strtoul(hex, NULL, 16);
              in += 3;  // Skip to end of hex code
              break;
            }
          }
          *out = *in;
          break;

        default: *out = *in; break;
      }
    } else { *out = *in; }

    out++;
    out_size--;
    in++;
  }

  // ? Does the null terminator count towards the character limit?
  // Spec says strings are max 255, but null-terminated internally. Implies
  // that the limit is only for source (aka external) string content.
  // However, a 256 char internal buffer would force an extra bit for addressing
  // instead of a clean 1 byte block (does this even matter?).
  if (*in == '"') {
    *out = '\0';
  }
  return 0;
}

// Maps byte values -> chars (with escaping)
void string_encode(const char *raw, char *out, size_t out_size) {
  assert(raw != NULL);
  assert(out != NULL);  // TODO: make these only active during debugging

  *out = '"';
  out++;
  out_size--;

  while (*raw && out_size > 1) {
    uint8_t byte = (uint8_t)(*raw);
    if (is_printable(byte)) {
      *out = *raw;
      out++;
      out_size--;
      raw++;
      continue;
    }

    switch (*raw) {
      case '\a': *out = '\\'; out++; *out = 'a'; break;
      case '\b': *out = '\\'; out++; *out = 'b'; break;
      case '\e': *out = '\\'; out++; *out = 'e'; break;
      case '\f': *out = '\\'; out++; *out = 'f'; break;
      case '\n': *out = '\\'; out++; *out = 'n'; break;
      case '\r': *out = '\\'; out++; *out = 'r'; break;
      case '\t': *out = '\\'; out++; *out = 't'; break;
      case '\v': *out = '\\'; out++; *out = 'v'; break;
      case '\\': *out = '\\'; out++; *out = '\\'; break;
      case '\'': *out = '\\'; out++; *out = '\''; break;
      case '"': *out = '\\'; out++; *out = '"'; break;
      default:
        *out = '\\'; out++; *out = '0'; out++; *out = 'x'; out++;
        byte_to_hexstr(&out, byte);
        break;
    }
    out++;
    out_size--;
    raw++;
  }

  *out = '"';
  out++;
  *out = '\0';

  return;
}
