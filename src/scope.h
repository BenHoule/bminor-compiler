#pragma once

void scope_enter();
void scope_exit();
int scope_depth();

struct symbol;

/* Binds name -> symbol  (innermost scope) */
int scope_bind(const char *name, struct symbol *symbol );

/* Searches innermost -> outermost. NULL if not found */
struct symbol * scope_lookup( const char *name );

/* Searches only innermost scope. Used for redeclaration checks */
struct symbol * scope_lookup_local( const char *name );
