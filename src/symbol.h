#pragma once

typedef enum {
  SYMBOL_LOCAL,    // `which` = stack slot index
  SYMBOL_PARAM,    // `which` = parameter index
  SYMBOL_GLOBAL    // `which` = unused
} symbol_t;

struct type;

struct symbol {
  symbol_t kind;
  struct type *type;
  char *name;
  int which;
  bool is_defined;  // Functions only
};

struct symbol * symbol_create( symbol_t kind, struct type *type, char *name );

void symbol_print_binding(const struct symbol *s);
