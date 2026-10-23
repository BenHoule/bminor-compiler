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

Individual tests may be passed directly to the compiler:
```sh
./bminor -scan test/scanner/good/keywords.bminor
```

Examples from the compiler book reside in a separate `book_tests/` directory in the project root and can be run with:
```sh
cd book_tests/
chmod +rwx run-tests.sh
./run-tests.sh ../bminor <-scan|-parse|etc...> ./
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
program   : decl_list
          ;

decl_list : decl decl_list
          | %empty
          ;

decl      : TOKEN_IDENTIFIER ':' var_type ';'
          | TOKEN_IDENTIFIER ':' var_type '=' init ';'
          | TOKEN_IDENTIFIER ':' func_type ';'
          | TOKEN_IDENTIFIER ':' func_type '=' block
          ;

/* ------- INITIALIZERS ------- */
init      : expr
          | '{' init_list '}'
          ;

init_list : init
          | init ',' init_list
          ;

/* ------- BLOCKS ------- */
block           : '{' block_item_list '}'
                ;

block_item_list : block_item block_item_list
                | %empty
                ;

block_item      : decl
                | stmt
                ;

/* ------- TYPES ------- */
func_type       : TOKEN_FUNCTION return_type '(' opt_param_list ')'
                ;

return_type     : primitive_type
                | TOKEN_VOID
                | TOKEN_AUTO
                ;

opt_param_list  : param_list
                | %empty
                ;

param_list      : param
                | param ',' param_list
                ;

param           : TOKEN_IDENTIFIER ':' param_type
                ;

param_type      : primitive_type         /* No lengths on array params */
                | TOKEN_ARRAY '[' ']' param_type
                | TOKEN_CARRAY '[' ']' param_type
                ;

var_type        : simple_type
                | TOKEN_AUTO
                ;

simple_type     : primitive_type
                | TOKEN_ARRAY '[' expr ']' simple_type
                | TOKEN_CARRAY '[' expr ']' simple_type
                ;

primitive_type  : TOKEN_BOOLEAN
                | TOKEN_CHAR
                | TOKEN_DOUBLE
                | TOKEN_INTEGER
                | TOKEN_STRING
                ;

/* ------- STATEMENTS ------- */
stmt            : matched_stmt
                | unmatched_stmt        /* Dangling else resolution */
                ;

matched_stmt    : simple_stmt
                | TOKEN_IF '(' expr ')' matched_stmt TOKEN_ELSE matched_stmt
                | TOKEN_FOR '(' opt_expr ';' opt_expr ';' opt_expr ')' matched_stmt
                ;

unmatched_stmt  : TOKEN_IF '(' expr ')' stmt
                | TOKEN_IF '(' expr ')' matched_stmt TOKEN_ELSE unmatched_stmt
                | TOKEN_FOR '(' opt_expr ';' opt_expr ';' opt_expr ')' unmatched_stmt
                ;

simple_stmt     : block
                | expr ';'
                | TOKEN_RETURN ';'
                | TOKEN_RETURN expr ';'
                | TOKEN_PRINT ';'
                | TOKEN_PRINT expr_list ';'
                ;

/* ------- EXPRESSIONS ------- */
opt_expr_list : expr_list
              | %empty
              ;

expr_list     : expr
              | expr ',' expr_list
              ;

opt_expr      : expr
              | %empty
              ;

expr          : or_expr
              | or_expr '=' expr
              ; /* right-associative */

or_expr       : and_expr
              | or_expr TOKEN_OR and_expr
              ; /* left-associative */

and_expr      : cmp_expr
              | and_expr TOKEN_AND cmp_expr
              ;

cmp_expr      : add_expr
              | cmp_expr '<' add_expr
              | cmp_expr '>' add_expr
              | cmp_expr TOKEN_LE add_expr
              | cmp_expr TOKEN_GE add_expr
              | cmp_expr TOKEN_EQ add_expr
              | cmp_expr TOKEN_NE add_expr
              ;

add_expr      : mul_expr
              | add_expr '+' mul_expr
              | add_expr '-' mul_expr
              ;

mul_expr      : unary_expr
              | mul_expr '*' unary_expr
              | mul_expr '/' unary_expr
              | mul_expr '%' unary_expr
              ;

unary_expr    : len_expr
              | '-' unary_expr
              | '!' unary_expr
              ;

len_expr      : pow_expr
              | '#' len_expr
              ;

pow_expr      : postfix_expr
              | postfix_expr '^' unary_expr        /* -x^2 -> -(x^2) */
              ;

postfix_expr  : primary_expr
              | postfix_expr TOKEN_INC
              | postfix_expr TOKEN_DEC
              | postfix_expr '[' expr ']'
              | postfix_expr '(' opt_expr_list ')'
              ;

primary_expr  : '(' expr ')'
              | TOKEN_IDENTIFIER
              | TOKEN_CHAR_LITERAL
              | TOKEN_DOUBLE_LITERAL
              | TOKEN_INTEGER_LITERAL
              | TOKEN_STRING_LITERAL
              | TOKEN_FALSE
              | TOKEN_TRUE
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
