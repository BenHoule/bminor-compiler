#include "string_util.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

constexpr uint8_t INVALID_HEX = 0xFF;  // (255)

// byte -> "\0xHH"
static void byte_to_hexstr(char *out, const uint8_t raw) {
  static constexpr char hex_digits[] = "0123456789ABCDEF";
  out[0] = hex_digits[(raw >> 4) & 0xF];  // High nibble
  out[1] = hex_digits[raw & 0xF];  // Low nibble
}

// hex char -> decimal value (0-15)
static uint8_t hexval(char c) {
  if ('0' <= c && c <= '9') return c - '0';  // [0...9]

  c |= 0x20;  // (uppercase | 0x20) ==> lowercase
  if ('a' <= c && c <= 'f') return c - 'a' + 10;  // [10...15]

  return INVALID_HEX;  // Bad hex char
}

uint8_t char_encode(const char **in) {
  assert(in != NULL);  //TODO: add -DNDEBUG to makefile for non-debug builds

  if (**in != '\\')
    return (uint8_t)**in;

  (*in)++;  // Skip the escape character
  switch (**in) {
    case 'a': return '\a';
    case 'b': return '\b';
    case 'e': return 0x1B;  // NOTE: '\e' not supported by MSVC - using 0x1B for portability
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    case '0':
      const char *hex = *in;
      if (hex[1] == 'x') {
        const uint8_t high = hexval(hex[2]);
        const uint8_t low = hexval(hex[3]);
        if (high == INVALID_HEX || low == INVALID_HEX)
          return '0';

        *in += 3;  // Skip to last character of the hex sequence
        return (uint8_t)((high << 4) | low);
      }
      [[fallthrough]];
    default: return (uint8_t)**in;
  }
}

// Maps chars -> byte values
size_t string_encode(const char *in, char *out, size_t out_size) {
  assert(in != NULL);
  assert(out != NULL);  //TODO: add -DNDEBUG to makefile for non-debug builds

  char *const start = out;
  in++;  // Skip the opening double quote

  while (*in != '"') {
    if (out_size <= 1) return ERR_TOO_LONG;  // Overflow

    const uint8_t byte = char_encode(&in);

    // 128-255 are undefined + no nulls in strings
    constexpr uint8_t DEL = 127;
    if (byte == 0 || DEL < byte) return ERR_INVALID_CHAR;

    *out = (char)byte;
    out++;
    out_size--;
    in++;  // `in` points to character following the one just encoded
  }

  *out = '\0';
  return (size_t)(out - start + 1);
}

// NOTE: Static internal buffer - e.g.
// printf("%s %s", char_decode(a,'\''), char_decode(b,'\''))
// prints the same character twice.
const char* char_decode(const char in, const char quote) {
  static char buf[6] = ""; // Enough for '\0xHH' + Null Terminator

  // NOTE: Since " and \ are ASCII 34 and 92, respectively, there is no
  // convenient way to filter them out of the printable character check.
  if (isprint(in)) {
    if (in == quote || in == '\\') {
      buf[0] = '\\';
      buf[1] = in;
      buf[2] = '\0';  // Null-terminate the escape sequence
    } else {
      buf[0] = in;
      buf[1] = '\0';  // Null-terminate the printable character
    }
    return buf;
  }

  buf[0] = '\\';
  switch (in) {
    case '\a': buf[1] = 'a'; buf[2] = '\0'; break;
    case '\b': buf[1] = 'b'; buf[2] = '\0'; break;
    case 0x1B: buf[1] = 'e'; buf[2] = '\0'; break;  // NOTE: '\e' not supported by MSVC - using 0x1B for portability
    case '\f': buf[1] = 'f'; buf[2] = '\0'; break;
    case '\n': buf[1] = 'n'; buf[2] = '\0'; break;
    case '\r': buf[1] = 'r'; buf[2] = '\0'; break;
    case '\t': buf[1] = 't'; buf[2] = '\0'; break;
    case '\v': buf[1] = 'v'; buf[2] = '\0'; break;
    default:
      buf[1] = '0';
      buf[2] = 'x';
      byte_to_hexstr(&buf[3], (uint8_t)in);
  }
  return buf;
}

// Maps byte values -> chars (with escaping)
size_t string_decode(const char *in, char *out, size_t out_size) {
  assert(in != NULL);
  assert(out != NULL);
  assert(out_size > 2);  //TODO: add -DNDEBUG to makefile for non-debug builds

  char *const start = out;
  *out = '"';
  out++;
  out_size--;

  while (*in) {
    const char *next = char_decode(*in, '\"');
    if (*next == '\0') return ERR_INVALID_CHAR;

    const size_t REQ_SPACE = strlen(next) + 3;  // Char + Closing quote + '\0'
    if(REQ_SPACE > out_size) return ERR_TOO_LONG;  // Overflow

    memcpy(out, next, strlen(next));
    out += strlen(next);
    out_size -= strlen(next);
    in++;
  }

  memcpy(out, "\"\0", 2);
  return (size_t)(out - start + 2);
}
