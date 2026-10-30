#include "bminor.tab.h"
#include "memory.h"
#include "expr.h"
#include "scope.h"
#include "symbol.h"
#include "type.h"

#include <stdio.h>
#include <stdlib.h>

static int failures = 0;
static unsigned int check_count = 0;

#define CHECK(condition) \
  do { \
    check_count++; \
    if (!(condition)) { \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
      failures++; \
    } \
  } while (0)

static void run_test(const char *name, void (*test_fn)(void)) {
  const unsigned int checks_before = check_count;
  const int failures_before = failures;

  printf("[ RUN      ] %s\n", name);
  test_fn();

  if (failures == failures_before) {
    printf("[       OK ] %s (%u checks)\n", name, check_count - checks_before);
  } else {
    printf("[  FAILED  ] %s (%u checks, %d failures)\n",
           name, check_count - checks_before, failures - failures_before);
  }
}

static void test_ast_contracts(void) {
  struct expr *value = expr_create_integer_literal(1);
  CHECK(!expr_validate(value));
  CHECK(expr_typecheck(value) == type_primitive(TYPE_INTEGER));
  CHECK(expr_validate(value));
  struct expr *list = expr_create(EXPR_ARG, value, nullptr);
  CHECK(expr_validate(list));
  CHECK(!type_validate(type_create_auto()));

  scope_enter();
  struct symbol *symbol = symbol_create(SYMBOL_GLOBAL, type_primitive(TYPE_INTEGER), "x");
  CHECK(scope_bind("x", symbol) != 0);
  struct expr *tree = nullptr;
  for (int i = 0; i < 10000; i++) {
    tree = expr_create(EXPR_ARG, expr_create_name("x"), tree);
  }
  expr_resolve(tree);
  int count = 0;
  for (struct expr *cell = tree; cell; cell = cell->right) {
    CHECK(cell->left->symbol == symbol);
    count++;
  }
  CHECK(count == 10000);
  scope_exit();
  CHECK(tree->left->symbol == symbol);
}

int main(void) {
  run_test("AST validation and non-mutating deep resolution", test_ast_contracts);
  memory_release();

  printf("\n1 test group, %u checks, %d failed\n", check_count, failures);
  if (failures != 0) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
