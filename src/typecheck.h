#pragma once

/**
 * @return The number of reported typecheck errors.
 */
unsigned int typecheck_error_count(void);

/**
 * @brief Reports a typecheck error with a formatted message.
 *
 * @param[in] fmt The format string.
 * @param[in] varg (...) Additional arguments for the format string.
 */
void typecheck_error(const char *fmt, ...);
