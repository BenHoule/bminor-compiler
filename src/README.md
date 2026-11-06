Starter Code for BMinor Project
-------------------------------

This directory contains the starter code for the BMinor compiler.
Students in COMP.4060 are *required* to use these header files as the basis for your work.
Trust us, it will get you started on the right track, and make it
much easier for the instructors to help you with any difficulties.

Of course, to complete the project, you will have to add quite a bit
to what is here.  You are free to add new files, add items to the
structures, implement new functions, and so forth.  But stick to
the basic structure that `decl` represents a declaration,
`expr` represents an expression, and so forth.

For your sanity and ours, you *must* adhere to the following style:
- Every function must be prefixed with the name of the structure that it manipulates.
- Function prototypes go in the `.h` file of the corresponding name.
- Function implementations go in the `.c` file of the corresponding name.

For example, the function `decl_print` prints out a declaration.
Its prototype is found in `decl.h` and the implementation is found in `decl.c`.

For the parsing assignment:

The include files `decl.h stmt.h expr.h type.h param_list.h`
comprise the abstract syntax tree (AST) for CMinor.
A CMinor program is a list of declarations (`decl`) of either global
variables or global functions.  A global function declaration
is a list of statements (`stmt`) such as if-else, while-loops,
and return statements.  Most statements contain expressions (`expr`)
which are trees of operators and values.  A type structure (`type`)
is used to represent abstract types like `integer`, `string`,
and `array of boolean`.

For the typechecking assignment:

The module `hash_table` implements a string-based hash table,
which will come in handy in the typechecking assignment.
The hash table will point to objects of type `symbol` so
as to match variables names (`x`) with their definitions,
like "parameter 3" or "global integer x".

For the code generation assignment:

The module 'library.c' contains the "standard library" for bminor,
which is needed to implement the print statement and the exponentiation
operator.  You are welcome to add items to the library if you find
it helpful.  Set up your Makefile carefully: library.c should be
linked against a compiled bminor program, not against the compiler itself.

Project implementation notes
----------------------------

The current command-line pipeline is scanner -> parser -> AST -> resolver ->
typechecker -> AST validation -> IR lowering. `-ir` prints the lowered IR and
`-interp` executes it. `-codegen` emits Linux x86-64 GNU assembler in AT&T
syntax using the System V AMD64 ABI. Assemble and link with GCC.
`library.c` and `library.h` provide the runtime ABI for generated programs;
compile the runtime with `-std=c23` and link it with the generated assembly
and `-lm`. See the [compile-and-run instructions](../README.md#compile-and-run-a-program).

Ownership:
- AST nodes, allocated types, symbols, and accepted token strings are owned by
  the compilation allocation registry in `memory.c`. The driver calls
  `memory_release()` after the last AST consumer, on successful and failed
  compilations. Do not free these objects individually.
- Declaration/parameter constructors take ownership of malloc-allocated token
  text. Expression name/string constructors copy their input; parser actions
  free the consumed token after copying. Unconsumed tokens are freed by Bison
  destructors, or by the scan-only driver.
- Scopes own hash-table keys/entries and borrow symbols. Exiting a scope must
  not destroy symbols: bound expressions still reference them.
- Expression type annotations, symbol types, and AST type fields may alias.
  Canonical primitive types are borrowed static objects; never mutate/free them.
- The scanner owns its Flex buffers; `scanner_close()` releases them.

AST contracts:
- Successful typechecking requires prior successful resolution. Compilation
  errors stop the pipeline before lowering. A read-only `decl_validate()` pass
  checks concrete types and bound names before any future IR consumer.
- `EXPR_ARG` is a list cell: `left` is an element, `right` is the next cell.
  `EXPR_INIT.left` points to initializer-list cells. Containers do not have a
  value type; their elements do.
- Read only union members appropriate to a node's kind. Resolver traversal
  uses an explicit stack and never temporarily rewires child pointers.
  The compiler still has global scanner/resolver state and is not reentrant.
- Type compatibility deliberately ignores array lengths and parameter names.
- Symbol `which` values are logical indices. `frame_locals` counts local
  declarations, not bytes or target-specific stack slots.
- Arithmetic constants are evaluated without rewriting the AST. Runtime-sized
  arrays remain allowed; lowering must validate their dimensions at runtime.

Expression resolver traversal
-----------------------------

The active `expr_resolve()` uses an explicit stack. It does not modify child
links and bounds C call-stack usage independently of AST height. The alternatives
below are documented for comparison and experimentation; the active implementation
should be changed only after measuring representative programs.

### Why not rely on tail-call optimization?

Tail-call optimization (TCO) is helpful when a recursive call is in tail
position: the caller has no work left after the call returns. It is not a
general replacement for a traversal stack. A binary-tree preorder traversal
must visit the left child and then still return to visit the right child, so
the left recursive call is not a tail call. The right-child call may be
written in tail position, but that does not eliminate stack growth along a
deep left branch. Also, C does not guarantee TCO; whether a compiler applies
it depends on the compiler, options, and exact function shape.

Thus, TCO can reduce stack use in suitable recursive formulations, but it does
not reliably bound stack use for arbitrary expression-tree shapes. A manual
stack makes pending work explicit and avoids depending on TCO. It uses O(h)
auxiliary pointers for traversal height `h`, and the storage is heap-allocated
and grows as needed.

### Explicit manual-stack alternative

This is the active implementation in `expr.c`. Keep it here as a reference if
experimenting with Morris traversal:

```c
void expr_resolve(struct expr *e) {
  if (!e) return;
  size_t count = 0, capacity = 32;
  struct expr **stack = malloc(capacity * sizeof *stack);
  if (!stack) {
    fprintf(stderr, "bminor: failed to allocate resolver stack\n");
    exit(EXIT_FAILURE);
  }
  stack[count++] = e;
  while (count) {
    e = stack[--count];
    if (e->kind == EXPR_NAME && !(e->symbol = scope_lookup(e->name)))
      resolver_error("Undefined identifier '%s'", e->name);
    if (count + 2 > capacity) {
      capacity *= 2;
      struct expr **grown = realloc(stack, capacity * sizeof *stack);
      if (!grown) {
        free(stack);
        fprintf(stderr, "bminor: failed to grow resolver stack\n");
        exit(EXIT_FAILURE);
      }
      stack = grown;
    }
    if (e->right) stack[count++] = e->right;
    if (e->left) stack[count++] = e->left;
  }
  free(stack);
}
```

### Morris preorder alternative

Morris traversal uses O(1) auxiliary pointer space by temporarily threading
AST links. That space bound does not imply it is faster: finding predecessors
and writing/restoring links adds work and memory writes. Whether it helps is
an empirical question.

To try it, replace the active `expr_resolve()` implementation in `expr.c` with
this function. It reports the same undefined-name error and visits each
expression once in root-left-right order:

```c
void expr_resolve(struct expr *e) {
  while (e) {
    if (!e->left) {
      if (e->kind == EXPR_NAME && !(e->symbol = scope_lookup(e->name))) {
        resolver_error("Undefined identifier '%s'", e->name);
      }
      e = e->right;
      continue;
    }

    struct expr *predecessor = e->left;
    while (predecessor->right && predecessor->right != e) {
      predecessor = predecessor->right;
    }

    if (!predecessor->right) {
      if (e->kind == EXPR_NAME && !(e->symbol = scope_lookup(e->name))) {
        resolver_error("Undefined identifier '%s'", e->name);
      }
      predecessor->right = e;
      e = e->left;
    } else {
      predecessor->right = nullptr;
      e = e->right;
    }
  }
}
```

Morris traversal has stricter preconditions than the stack version:

- The AST must be a tree, not a DAG with shared child nodes or a graph with
  cycles.
- No other thread, callback, diagnostic hook, or reentrant compiler operation
  may inspect or traverse the AST while links are temporarily threaded.
- The traversal must run to completion. Do not add early returns, `longjmp`,
  cancellation, or other exits that could leave a temporary thread installed.
- Keep the thread test `predecessor->right == e` exact: a real right child must
  not be mistaken for a traversal thread.

This affects AST-link safety only; the resolver still uses global scope and
error state, so this choice alone does not make the compiler thread-safe.

Runtime:
- `library.h` defines the ABI for generated programs, including 64-bit integers
  and double printing. Link `library.c` with generated code, not the compiler.
- Integer power uses checked exponentiation by squaring. Nonpositive exponents
  return 1, including `0^0`; overflow reports an error and exits.
- The current toolchain assumes GCC-compatible C23 and overflow builtins.

Source spans in resolver/typechecker diagnostics and a minimal-parentheses
printer remain separate follow-ups.
Parser/scanner locations are already tracked; propagating spans through AST
construction requires a deliberate diagnostic-output change.
