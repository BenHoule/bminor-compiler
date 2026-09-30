#include "string_util.h"

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static constexpr uint8_t SPACE = 32;
static constexpr uint8_t DEL = 127;
static constexpr uint8_t ERR_INVALID_HEX = 0xFF;  // (255)

static inline bool is_printable(const uint8_t byte) {
  // spec says "between 32 and 127" but spaces need to be included
  // while DEL is not printable(?).
  return SPACE <= byte && byte < DEL;
}

// byte -> "\0xHH"
static void byte_to_hexstr(unsigned char *out, const uint8_t raw) {
  // constexpr char hex_digits[] = "0123456789ABCDEF";  <--(c23)
  static constexpr unsigned char hex_digits[] = "0123456789ABCDEF";
  out[3] = hex_digits[(raw >> 4) & 0xF];  // High nibble
  out[4] = hex_digits[raw & 0xF];  // Low nibble
}

//
static inline uint8_t hexval(unsigned char c) {
  if ('0' <= c && c <= '9') return c - '0';  // [0...9]

  c |= 0x20;  // (uppercase | 0x20) ==> lowercase
  if ('a' <= c && c <= 'f') return c - 'a' + 10;  // [10...15]

  return ERR_INVALID_HEX;  // Bad hex char
}

uint8_t char_encode(const unsigned char **in) {
  assert(in != NULL);  //TODO: add -DNDEBUG to makefile for non-debug builds
   const unsigned char *p = *in;  // Save the original pointer
  if (*p != '\\'){
    *in = p + 1;
    return *p;
  }
  p++;  // Move past the escape character
  uint8_t result; // Store the decoded character
  switch (*p) {
    case 'a': result = '\a'; break;
    case 'b': result = '\b'; break;
    case 'e': result = '\e'; break;
    case 'f': result = '\f'; break;
    case 'n': result = '\n'; break;
    case 'r': result = '\r'; break;
    case 't': result = '\t'; break;
    case 'v': result = '\v'; break;
    case '0':
      if (in[1] == 'x') {
        const uint8_t high = hexval(in[2]);
        if (high == ERR_INVALID_HEX) return (uint8_t)*in;

        const uint8_t low = hexval(in[3]);
        if (low == ERR_INVALID_HEX) return (uint8_t)*in;

        *in = (unsigned char*)(p + 4);  // Skip to last character of the hex sequence
        return (uint8_t)(high << 4) | low;
      }
      [[fallthrough]];
    default:
    result = *p;
    break;
  }
  *in = p + 1;  // Skip to the character following the escape sequence
  return result;
}

// Maps chars -> byte values
size_t string_encode(const unsigned char *in, uint8_t *out, size_t out_size) {
  assert(in != NULL);
  assert(out != NULL);  //TODO: add -DNDEBUG to makefile for non-debug builds

  unsigned char *const start = out;
  in++;  // Skip the opening double quote

  [[maybe_unused]] static constexpr size_t ERR_INVALID_CHAR = 0;
  [[maybe_unused]] static constexpr size_t ERR_TOO_LONG = SIZE_MAX;
  while (*in != '"' && *(in + 1)) {
    if (out_size <= 1) return ERR_TOO_LONG;  // Buffer overflow

    *out = char_encode(&in);
    if (*out > DEL) return ERR_INVALID_CHAR;  // 128-255 are undefined

    out++;
    out_size--;

  }

  *out = '\0';
  return (size_t)(out - start) + 1;
}

// Maps byte values -> chars (with escaping)
size_t string_decode(const uint8_t *in, unsigned char *out, size_t out_size) {
  assert(in != NULL);
  assert(out != NULL);
  assert(out_size > 2);  //TODO: add -DNDEBUG to makefile for non-debug builds

  unsigned char *const start = out;
  *out = '"';
  out++;
  out_size--;

  while (*in && out_size > 1) {
    const uint8_t byte = (uint8_t)(*in);
    if (is_printable(byte)) {
      *out = *in;
      out++;
      out_size--;
      in++;
      continue;
    }

    static constexpr uint8_t ESC_SEQ_REQ_SPACE = 4;  // '\\', 'n', '"', '\0'
    if (out_size < ESC_SEQ_REQ_SPACE) return 0;  // Buffer overflow

    static constexpr uint8_t HEXSTR_REQ_SPACE = 7;  // '\\', '0', 'x', 'H', 'H', '"', '\0'
    switch (*in) {
      case '\a': memcpy(out, "\\a", 2); break;
      case '\b': memcpy(out, "\\b", 2); break;
      case '\e': memcpy(out, "\\e", 2); break;
      case '\f': memcpy(out, "\\f", 2); break;
      case '\n': memcpy(out, "\\n", 2); break;
      case '\r': memcpy(out, "\\r", 2); break;
      case '\t': memcpy(out, "\\t", 2); break;
      case '\v': memcpy(out, "\\v", 2); break;
      case '\\': memcpy(out, "\\\\", 2); break;
      case '\'': memcpy(out, "\\\'", 2); break;
      case '\"': memcpy(out, "\\\"", 2); break;
      default:
        if (out_size < HEXSTR_REQ_SPACE) return 0;  // Buffer overflow
        memcpy(out, "\\0x", 3);
        byte_to_hexstr(out, byte);
        out += 3;  // +3 for "xHH"
        out_size -= 3;
    }
    out += 2;  // +2 for escape sequence (e.g. "\n")
    out_size -= 2;
    in++;
  }

  memcpy(out, "\"\0", 2); //? Is null terminator necessary here?
  out += 2;
  out_size -= 2;  // Account for the closing quote and null terminator

  return (size_t)(out - start);
}
