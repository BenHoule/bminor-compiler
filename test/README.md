# Testing
```sh
make test
make valgrind
```
`make test` runs unit, stage, printer round-trip, resolver-position, and book
tests. `make valgrind` runs unit checks and the stage regression driver under
Valgrind, not the printer/resolver scripts or book tests.
`make sanitize` runs the complete test target with an isolated ASan/UBSan build.
The test drivers accept `BMINOR=/absolute/path/to/compiler`.

The main `test/` directory is grouped by compiler stage, with passing and failing
cases in separate directories:
```sh
test/
  scanner/
    good/
      <test>.bminor
      expected/<test>.expected  # optional exact-output baseline
      out/<test>.out            # generated test output
    bad/
      <test>.bminor
      expected/<test>.expected
      out/<test>.out
  parser/
    ...
```

Good cases must exit 0; bad cases must exit 1, not crash or fail with a usage
error. If a matching file exists under `expected/`, output must match exactly.
`make clean` removes generated `out/` directories.

### Printer and constant-expression regressions

The printer fixtures `unary_power_grouping.bminor` and
`double_literal_types.bminor` have exact-output baselines. They distinguish
`(-x)^2` from `-(x^2)`, left- from right-associated powers, nested unary
operators, and integer-looking double literals. These baselines complement
the idempotence checks in `printer/roundtrip.sh`: stable output alone does
not prove that the original expression's meaning was preserved.

The typechecker fixtures named `constant_*`, `computed_array_*`,
`computed_initializer_count.bminor`, and `runtime_array_sizes.bminor`
describe the intended arithmetic-constant behavior:

- Integer `+`, `-`, `*`, `/`, `%`, `^`, and nested numeric negation are
  evaluated without signed overflow. Double arithmetic supports `+`, `-`,
  `*`, `/`, and negation.
- Division by zero, integer overflow, and non-finite double results report
  errors. Negative integer exponents and `0^0` produce `1`, matching the
  existing `integer_power` runtime helper.
- Known array sizes must be positive and match initializer counts.
  Runtime-dependent integer sizes remain allowed; no compile-time
  positivity or initializer-count claim is made for those sizes.
- Names and calls are not arithmetic constants. This does not introduce
  constant propagation, logical/comparison evaluation, or AST rewriting.

Error baselines check the diagnostic so that rejecting every arithmetic expression as "nonconstant" cannot falsely pass them.

Individual tests may be passed directly to the compiler:
```sh
./bminor -scan test/scanner/good/keywords.bminor
```

Examples from the compiler book reside in a separate `book_tests/` directory in the project root and can be run with:
```sh
cd book_tests
./run-tests.sh ../bminor -parse parser
```
