#include "bminor.tab.h"
#include "scanner.h"
#include "string_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void test_string_encoding(void) {
  const char *literal = "\"a\\n\\\"\\\\\"";
  char encoded[16] = {0};
  const size_t encoded_size = string_encode(literal, encoded, sizeof(encoded));
  const char expected[] = {'a', '\n', '"', '\\', '\0'};

  CHECK(encoded_size == sizeof(expected));
  CHECK(memcmp(encoded, expected, sizeof(expected)) == 0);

  char decoded[16] = {0};
  const size_t decoded_size = string_decode(encoded, decoded, sizeof(decoded));
  CHECK(decoded_size == strlen(literal) + 1);
  CHECK(strcmp((const char *)decoded, literal) == 0);

  const char nul_literal[] = "\"\\0x00\"";
  CHECK(string_encode(nul_literal, encoded, sizeof(encoded)) == ERR_INVALID_CHAR);

  char max_literal[258] = {'"'};
  memset(max_literal + 1, 'a', 255);
  max_literal[256] = '"';
  max_literal[257] = '\0';
  char max_encoded[256] = {0};
  CHECK(string_encode(max_literal, max_encoded, sizeof(max_encoded)) == sizeof(max_encoded));

  char overlong_literal[259] = {'"'};
  memset(overlong_literal + 1, 'a', 256);
  overlong_literal[257] = '"';
  overlong_literal[258] = '\0';
  CHECK(string_encode(overlong_literal, max_encoded, sizeof(max_encoded)) == ERR_TOO_LONG);
}

static void test_string_decode_boundaries(void) {
  const char printable[] = {'a', '\0'};
  char guarded[4] = {'?', '?', '?', '!'};
  const size_t short_size = string_decode(printable, guarded, 3);
  CHECK(short_size == ERR_TOO_LONG);
  CHECK(guarded[3] == '!');

  const char quote[] = {'"', '\0'};
  char quote_output[6] = {0};
  const size_t quote_size = string_decode(quote, quote_output, 4);
  CHECK(quote_size == ERR_TOO_LONG);
}

static void test_scanner_values(void) {
  FILE *input = tmpfile();
  if (input == NULL) {
    fprintf(stderr, "%s:%d: tmpfile failed\n", __FILE__, __LINE__);
    failures++;
    return;
  }

  fputs("0b101 0xFF 42 12.5e1 \"a\\nb\"", input);
  rewind(input);
  scanner_open(input);

  CHECK(scanner_next_token() == TOKEN_INTEGER_LITERAL);
  CHECK(yylval.ival == 5);
  CHECK(scanner_next_token() == TOKEN_INTEGER_LITERAL);
  CHECK(yylval.ival == 255);
  CHECK(scanner_next_token() == TOKEN_INTEGER_LITERAL);
  CHECK(yylval.ival == 42);
  CHECK(scanner_next_token() == TOKEN_DOUBLE_LITERAL);
  CHECK(yylval.dval == 125.0);
  CHECK(scanner_next_token() == TOKEN_STRING_LITERAL);
  CHECK(yylval.sval != NULL);
  if (yylval.sval != NULL) {
    CHECK(strcmp(yylval.sval, "a\nb") == 0);
    free(yylval.sval);
  }
  CHECK(scanner_next_token() == 0);
  CHECK(scanner_error_count() == 0);

  fclose(input);
}

int main(void) {
  run_test("String encoding, escapes, and length limits", test_string_encoding);
  run_test("String decoding buffer boundaries", test_string_decode_boundaries);
  run_test("Scanner token values and ownership", test_scanner_values);

  printf("\n3 test groups, %u checks, %d failed\n", check_count, failures);
  if (failures != 0) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
