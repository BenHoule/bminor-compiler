# Testing
```sh
make test
make valgrind
```
Will run all test suites in sequence, with/without valrind. The main `test/` directory is grouped by compiler stage, with passing and failing cases in separate directories:
```sh
test/
  scanner/
    good/<test>.bminor
      expected/<test>.expected  # optional exact-output baseline
      out/<test>.out            # generated test output
    bad/<test>.bminor
      expected/<test>.expected
      out/<test>.out
  parser/
    ...
```

Each case must have the expected exit status. If its matching file exists under `expected/`, the program output must match exactly. `make clean` removes generated `out/` directories.

Scanner examples from the compiler book remain separate under `test/scanner/book_tests/` and can be run with:
```sh
./test/scanner/book_tests/run_all_tests.sh
```

Individual tests may be passed directly to the compiler:
```sh
./bminor -scan test/scanner/good/keywords.bminor
```
