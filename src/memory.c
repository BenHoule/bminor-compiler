#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct memory_entry {
  void *pointer;
  struct memory_entry *next;
};

static struct memory_entry *allocations;

void memory_take(void *pointer) {
  if (!pointer) return;

  struct memory_entry *entry = malloc(sizeof *entry);
  if (!entry) {
    fprintf(stderr, "bminor: failed to track compilation allocation\n");
    free(pointer);
    memory_release();
    exit(EXIT_FAILURE);
  }

  *entry = (struct memory_entry){pointer, allocations};
  allocations = entry;
}

void *memory_allocate(size_t size) {
  void *pointer = calloc(1, size);
  if (!pointer) {
    fprintf(stderr, "bminor: failed to allocate compilation memory\n");
    memory_release();
    exit(EXIT_FAILURE);
  }

  memory_take(pointer);
  return pointer;
}

char *memory_copy_string(const char *text) {
  size_t size = strlen(text) + 1;
  char *copy = memory_allocate(size);
  memcpy(copy, text, size);
  return copy;
}

void memory_release(void) {
  while (allocations) {
    struct memory_entry *entry = allocations;
    allocations = entry->next;
    free(entry->pointer);
    free(entry);
  }
}
