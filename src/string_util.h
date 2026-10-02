#pragma once

#include <stdlib.h>
#include <stdint.h>

constexpr size_t ERR_INVALID_CHAR = 0;
constexpr size_t ERR_TOO_LONG = SIZE_MAX;

uint8_t char_encode(const char **in);
size_t string_encode(const char *in, char *out, size_t out_size);
size_t string_decode(const char *in, char *out, size_t out_size);
