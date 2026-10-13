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

Individual tests may be passed directly to the compiler:
```sh
./bminor -scan test/scanner/good/keywords.bminor
```

Examples from the compiler book reside in a separate `book_tests/` directory in the project root and can be run with:
```sh
cd book_tests
chmod +rwx run-tests.sh
./run-tests.sh ../bminor <-scan|-parse|etc...> ./
```
