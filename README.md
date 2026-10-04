# COMP.4060 - COMPILER CONSTRUCTION: BMINOR
### Install & Build
```sh
git clone https://github.com/BenHoule/bminor-compiler
cd bminor-compiler/
make
make test
./bminor -scan test/scanner/good/keywords.bminor
```
**NOTE:** Makefile uses gcc-14 and `-std=c23` / `-std=gnu23` flags

---
### Testing
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

---
### Contributing
Make a branch
```sh
git checkout -b <branch>
```

*...make changes...*

Push changes
```sh
git add .
git commit -m "message"
git checkout main
git pull
git checkout <branch>
git rebase main
git push [--force-with-lease]
git checkout main
git merge --ff-only <branch>
git push
```

---
### Grammar
```bison
program     : expr ';'
            ;

opt_list    : %empty
            | expr_list
            ;

expr_list   : expr ',' expr_list
            | expr
            ;

expr        : assign_expr
            ;

assign_expr : or_expr '=' assign_expr        /* right-associative */
            | or_expr
            ;

or_expr     : or_expr TOKEN_OR and_expr      /* left-associative */
            | and_expr
            ;

and_expr    : and_expr TOKEN_AND cmp_expr
            | cmp_expr
            ;

cmp_expr    : cmp_expr '<' add_expr
            | cmp_expr '>' add_expr
            | cmp_expr TOKEN_LE add_expr
            | cmp_expr TOKEN_GE add_expr
            | cmp_expr TOKEN_EQ add_expr
            | cmp_expr TOKEN_NE add_expr
            | add_expr
            ;

add_expr    : add_expr '+' mul_expr
            | add_expr '-' mul_expr
            | mul_expr
            ;

mul_expr    : mul_expr '*' unary_expr
            | mul_expr '/' unary_expr
            | mul_expr '%' unary_expr
            | unary_expr
            ;

unary_expr  : '-' unary_expr
            | '!' unary_expr
            | len_expr
            ;

len_expr    : '#' len_expr
            | pow_expr
            ;

pow_expr    : postfix_expr '^' unary_expr    /* -x^2 -> -(x^2) */
            | postfix_expr                   /* see note below */
            ;

postfix_expr: postfix_expr TOKEN_INC
            | postfix_expr TOKEN_DEC
            | postfix_expr '[' expr ']'
            | postfix_expr '(' opt_list ')'
            | primary_expr
            ;

primary_expr: TOKEN_IDENTIFIER
            | TOKEN_CHAR_LITERAL
            | TOKEN_DOUBLE_LITERAL
            | TOKEN_INTEGER_LITERAL
            | TOKEN_STRING_LITERAL
            | TOKEN_FALSE
            | TOKEN_TRUE
            | '(' expr ')'
            ;
```
**NOTE:**  bottom-up structure (upper "consists of" lower); e.g. A `unary_expr` (-, !) consists of a `pow_expr` (^):
```
" -x^2 "
unary_expr
-> '-' len_expr
-> '-' pow_expr
-> '-' (postfix_expr '^' unary_expr)
-> '-' (primary_expr '^' len_expr)
-> '-' (TOKEN_NAME   '^' pow_expr)
-> '-' (TOKEN_NAME   '^' postfix_expr)
-> '-' (TOKEN_NAME   '^' primary_expr)
-> '-' (TOKEN_NAME   '^' TOKEN_INTEGER_LITERAL)
-> semantically " -(x^2) "
```
`unary_expr` has no path to `primary_expr` that does not pass through `pow_expr`. Were it the other way around, the derivation would look like:
```
" -x^2 "
pow_expr
-> unary_expr '^' pow_expr
-> ('-' unary_expr)   '^' unary_expr
-> ('-' len_expr)     '^' len_expr
-> ('-' postfix_expr) '^' postfix_expr
-> ('-' primary_expr) '^' primary_expr
-> ('-' TOKEN_NAME)   '^' TOKEN_INTEGER_LITERAL
-> semantically " (-x)^2 "
```
