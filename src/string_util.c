#include "string_util.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static inline bool is_printable(const uint8_t byte) {
  // spec says "between 32 and 127" but spaces need to be included
  // while DEL is not printable(?).
  constexpr int SPACE = 32;
  constexpr int DEL = 127;
  return SPACE <= byte && byte < DEL;
}

static inline void byte_to_hexstr(char **out, const uint8_t raw) {
  static constexpr char hex_digits[] = "0123456789ABCDEF";
  **out = hex_digits[(raw >> 4) & 0xF];  // High nibble
  (*out)++;
  **out = hex_digits[raw & 0xF];  // Low nibble
}

static inline void hexstr_to_byte(const char *const in, char *out) {
  assert(in != NULL);
  assert(out != NULL);

  char hex[3] = {0};
  if (isxdigit(in[0]) && isxdigit(in[1])) {
    hex[0] = in[2];
    hex[1] = in[3];
    *out = (char)strtoul(hex, NULL, 16);
  }
}

bool char_encode(const char *in, char *out) {
  assert(in[0] == '\'');
  assert(out != NULL);  //TODO: add -DNDEBUG to makefile for non-debug builds

  in++;
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
      case '0':
        if (in[1] == 'x') {
          hexstr_to_byte(in, out);
          in += 3;  // Skip to last character of the hex sequence
          break;
        }
        [[fallthrough]];
      default: *out = *in;
    }
  } else *out = *in;

  out++;
  in++;

  if (*in != '\'') {
    return false;  // char literal is too long
  }

  return true;
}

// Maps chars -> byte values
size_t string_encode(const char *in, char *out, size_t out_size) {
  assert(in[0] == '"' || in[0] == '\'');
  assert(out != NULL);  //TODO: add -DNDEBUG to makefile for non-debug builds

  char *const start = out;
  in++;
  while (*in && *in != '"' && *in != '\'') {
    if (out_size == 0) {
      return 0;  // Buffer overflow
    }

    if (*in == '\\' && in[1]) {
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
        case '0':
          if (in[1] == 'x') {
            hexstr_to_byte(in, out);
            in += 3;  // skip to last character of the hex sequence
            break;
          }
          [[fallthrough]];
        default: *out = *in;
      }
    } else *out = *in;

    out++;
    out_size--;
    in++;
  }

  *out = '\0';
  return (size_t)(out - start) + 1;
}

// Maps byte values -> chars (with escaping)
void string_decode(const char *raw, char *out, size_t out_size) {
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
        memcpy(out, "\\0x", 3);
        out += 3;
        byte_to_hexstr(&out, byte);
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
