#pragma once

#include <stdlib.h>
#include <stdint.h>

constexpr size_t ERR_INVALID_CHAR = 0;
constexpr size_t ERR_TOO_LONG = SIZE_MAX;

/**
 * @brief Encodes a single character (B-minor -> C).
 *
 * @param[in,out] in Pointer into a scanner-validated character/escape sequence.
 *
 * @return Encoded character as uint8_t.
 */
uint8_t char_encode(const char **in);

/**
 * @brief Encodes a string (B-minor -> C).
 *
 * @param[in] in Scanner-validated, quoted source literal.
 * @param[out] out Pointer to the output buffer.
 * @param[in] out_size Size of the output buffer.
 *
 * @return Bytes written including NUL, or ERR_TOO_LONG/ERR_INVALID_CHAR.
 */
size_t string_encode(const char *in, char *out, size_t out_size);

/**
 * @brief Decodes a single character (C -> B-minor).
 *
 * @param[in] in Encoded character.
 * @param[in] quote Quote character used for encoding.
 *
 * @param[out] out Caller-owned buffer of at least six bytes.
 */
void char_decode(char in, char quote, char out[6]);

/**
 * @brief Decodes a string (C -> B-minor).
 *
 * @param[in] in Pointer to the encoded input string.
 * @param[out] out Pointer to the output buffer.
 * @param[in] out_size Size of the output buffer.
 *
 * @return Bytes written including quotes and NUL, or ERR_TOO_LONG.
 * @pre out_size is at least three bytes.
 */
size_t string_decode(const char *in, char *out, size_t out_size);
