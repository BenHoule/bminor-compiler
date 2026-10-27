#pragma once

#include <stdlib.h>
#include <stdint.h>

constexpr size_t ERR_INVALID_CHAR = 0;
constexpr size_t ERR_TOO_LONG = SIZE_MAX;

/**
 * @brief Encodes a single character (B-minor -> C).
 *
 * @param[in] in Pointer to the input character.
 *
 * @return Encoded character as uint8_t.
 */
uint8_t char_encode(const char **in);

/**
 * @brief Encodes a string (B-minor -> C).
 *
 * @param[in] in Pointer to the input string.
 * @param[out] out Pointer to the output buffer.
 * @param[in] out_size Size of the output buffer.
 *
 * @return Number of characters written to the output buffer, or ERR_TOO_LONG on failure.
 */
size_t string_encode(const char *in, char *out, size_t out_size);

/**
 * @brief Decodes a single character (C -> B-minor).
 *
 * @param[in] in Encoded character.
 * @param[in] quote Quote character used for encoding.
 *
 * @return Decoded character as a C string.
 *
 * @warning Static internal buffer - e.g.
 * `printf("%s %s", char_decode(a,'\''), char_decode(b,'\''))`
 * prints the same character twice.
 */
const char* char_decode(const char in, const char quote);

/**
 * @brief Decodes a string (C -> B-minor).
 *
 * @param[in] in Pointer to the encoded input string.
 * @param[out] out Pointer to the output buffer.
 * @param[in] out_size Size of the output buffer.
 *
 * @return Number of characters written to the output buffer, or ERR_TOO_LONG on
 * failure.
 */
size_t string_decode(const char *in, char *out, size_t out_size);
