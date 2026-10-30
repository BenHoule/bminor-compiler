#pragma once

extern bool resolver_trace;  // Toggles printing binding trace

/**
 * @return The number of recorded resolver errors.
 */
unsigned int resolver_error_count(void);

/**
 * @brief Reports a resolver error with a formatted message.
 *
 * @param[in] fmt The format string (printf-style).
 * @param[in] varg (...) Additional arguments for the format string.
 */
[[gnu::format(printf, 1, 2)]]
void resolver_error(const char *fmt, ...);
