#pragma once

#include "type.h"
#include "stmt.h"
#include "expr.h"

struct decl {
  char *name;               // The name of the declaration
  struct type *type;        // The type of the declaration
  struct expr *value;       // The initial value of the declaration
  struct stmt *code;        // The code block associated with the declaration (for functions)
  struct symbol *symbol;    // The symbol table entry for the declaration
  struct decl *next;        // The next declaration in the list
};

struct decl * decl_create( char *name, struct type *type, struct expr *value, struct stmt *code, struct decl *next );
void decl_print( struct decl *d, int indent );
