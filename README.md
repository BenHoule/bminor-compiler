# COMP.4060 - Compiler Construction: BMinor

## Install and build

Requirements: GCC 14 (C23), GNU Make 4.3+, Flex, Bison, and Bash.
Valgrind and clang-format are optional development tools.

```sh
git clone https://github.com/BenHoule/bminor-compiler
cd bminor-compiler
make
make test
./bminor -scan test/scanner/good/keywords.bminor
```

## Status and command-line interface

```sh
./bminor <stage> <input-file>
```

| Stage | Behavior |
|-------|----------|
| `-scan` | Print token names |
| `-parse` | Check syntax and report success |
| `-print` | Print a parseable, conservatively parenthesized program |
| `-resolve` | Bind identifiers and print symbol positions |
| `-typecheck` | Resolve, typecheck, and validate the typed AST |
| `-codegen` | Check the frontend, then report that generation is unimplemented |

Exit codes: **0** for success, **1** for compile/input errors, **2** for CLI
usage errors.

The pipeline is scanner -> parser -> AST -> resolver -> typechecker -> future IR.
The [source directory guide](src/README.md) documents ownership, AST invariants,
and the generated-program ABI. Starter filenames and the AST-centered structure
are retained.

Flex and Bison generate files under `build/`; edit their source specifications,
not the generated C files. The [runtime library](src/library.c) links with
generated programs, never with the compiler.

## Development and testing

```sh
make unit           # scanner/string, AST, and runtime unit checks
make test           # all unit, stage, printer, resolver-position, and book tests
make valgrind       # unit and stage regressions under Valgrind
make release        # optimized binary: build/release/bminor
make sanitize       # isolated ASan/UBSan build and complete test suite
make analyze        # GCC static analysis of handwritten C
make format-check   # optional clang-format check; existing files need formatting
```

Release and sanitizer builds use separate directories to avoid mixing objects.
`CPPFLAGS`, `CFLAGS`, `GENFLAGS`, `LDFLAGS`, and `LDLIBS` can be overridden.
The default compiler is `gcc-14`; use `make CC=...` to override it with another
compatible compiler.

Regression cases live under `test/<stage>/{good,bad}/`:

```text
test/scanner/
  good/
    keywords.bminor
    expected/keywords.expected  # optional exact-output baseline
    out/keywords.out            # generated output
  bad/
    ...
```

Good cases must exit 0; bad cases must exit 1, not crash. Optional expected-output
files must match exactly. Printer checks include exact precedence/literal
fixtures, idempotence, binding traces, and re-typechecking of well-typed inputs.
See the [test guide](test/README.md) for constant-expression cases.

Select stages or a different binary with:

```sh
BMINOR_STAGES="printer typechecker" test/runtests.sh
BMINOR="$PWD/build/release/bminor" test/runtests.sh
book_tests/run-tests.sh ./bminor -parse book_tests/parser
```

`make clean` removes build artifacts, generated test `out/` directories, and
book-test `.bminor.out` files.

## Tooling

- **EditorConfig / clang-format:** shared formatting settings; keep bulk
  formatting separate from behavioral edits. The format target is opt-in until
  the existing formatting baseline is normalized.
- **Bear / clangd:** `bear -- make -B` produces a compilation database with the
  real include paths and generated-header locations. Use C23-capable versions.
- **GCC `-fanalyzer`:** `make analyze` compiles handwritten sources for analysis.
- **Cppcheck / ShellCheck:** optional complementary C and shell checks.
- **ASan / UBSan / Valgrind:** check memory lifetimes and undefined behavior.

CI runs the complete test suite, Valgrind regressions, GCC analysis, and
sanitizer tests.

## Grammar and precedence

The authoritative grammar is [bminor.bison](src/bminor.bison); token definitions
and literal rules are in [scanner.flex](src/scanner.flex).

The grammar uses explicit precedence layers and matched/unmatched statements
to resolve dangling `else`. Exponentiation is right-associative and binds more
tightly than unary minus:

```text
-x^2     means -(x^2)
(-x)^2   means (-x) raised to 2
2^3^2    means 2^(3^2)
```

Printing preserves these distinctions and keeps double literals distinguishable
from integer literals.

## Contributing

Create a branch, make focused changes, and run the relevant checks:

```sh
git switch -c <branch>
make test
git add <changed-files>
git commit -m "Describe the change"
git push -u origin <branch>
```

Open a pull request for review. Keep starter filenames and attribution intact;
do not include generated artifacts or unrelated formatting in behavior fixes.
