/**
 * @file memory.h
 * @brief Arena-style tracking for compiler-owned allocations.
 *
 * Registered pointers are released together by memory_release(). The registry
 * owns the allocation itself, but does not manage resources referenced by that
 * allocation. Do not individually free a registered pointer.
 */
#pragma once

#include <stddef.h>

/**
 * @brief Allocates zero-initialized memory and registers it for release.
 *
 * @param size Number of bytes to allocate.
 *
 * @return Pointer to the registered allocation.
 *
 * @note Allocation failure is fatal; diagnostics are written to stderr and
 *       registered allocations are released before the process exits.
 */
void *memory_allocate(size_t size);

/**
 * @brief Copies a NUL-terminated string into registered memory.
 *
 * @param text String to copy; must not be nullptr.
 *
 * @return Pointer to the registered copy.
 *
 * @note Allocation failure is fatal; see memory_allocate().
 */
char *memory_copy_string(const char *text);

/**
 * @brief Registers an existing heap allocation for release.
 *
 * @param pointer Heap pointer to take ownership of. nullptr is ignored.
 *
 * @warning The pointer must be safe to pass to free(). Do not use or free it
 *          after memory_release().
 *
 * @note If registration fails, pointer and all previously registered
 *       allocations are freed before the process exits.
 */
void memory_take(void *pointer);

/**
 * @brief Frees all allocations registered since the previous release.
 *
 * Safe to call when there are no registered allocations. After this call,
 * registered pointers are invalid; the registry can be reused for a later
 * compiler invocation.
 */
void memory_release(void);
