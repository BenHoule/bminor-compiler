#include "scanner.h"
#include "bminor.tab.h"
#include "bminor_parser.h"
#include "decl.h"
#include "memory.h"
#include "resolver.h"
#include "scope.h"
#include "typecheck.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  STAGE_SCAN,
  STAGE_PARSE,
  STAGE_PRINT,
  STAGE_RESOLVE,
  STAGE_TYPECHECK,
  STAGE_CODEGEN,
  STAGE_INVALID
} stage_t;

#define EXIT_COMPILE_ERROR 1  // Bad Input program
#define EXIT_USAGE_ERROR 2    // Bad CLI command

static stage_t stage_from_flag(const char *flag) {
  static const struct {
    const char *flag;
    stage_t stage;
  } flags[] = {
    {"-scan", STAGE_SCAN},
    {"-parse", STAGE_PARSE},
    {"-print", STAGE_PRINT},
    {"-resolve", STAGE_RESOLVE},
    {"-typecheck", STAGE_TYPECHECK},
    {"-codegen", STAGE_CODEGEN}
  };
  for (size_t i = 0; i < sizeof flags / sizeof *flags; i++)
    if (strcmp(flag, flags[i].flag) == 0) return flags[i].stage;
  return STAGE_INVALID;
}

static void print_usage(const char *program) {
  fprintf(stderr, "Usage: %s <stage> <input file>\n"
          "Stages: -scan -parse -print -resolve -typecheck\n"
          "        -codegen (not implemented)\n", program);
}

static int compile(stage_t stage, const char *flag) {
  if (stage == STAGE_SCAN) {
    int token;
    while ((token = scanner_next_token()) != 0) {
      printf("%s\n", token_name(token));
      if (token == TOKEN_STRING_LITERAL || token == TOKEN_IDENTIFIER)
        free(yylval.sval);
    }
    return scanner_error_count() ? EXIT_COMPILE_ERROR : EXIT_SUCCESS;
  }

  if (yyparse() || scanner_error_count() || parser_error_count())
    return EXIT_COMPILE_ERROR;

  if (stage == STAGE_PARSE) {
    printf("parse successful\n");
    return EXIT_SUCCESS;
  }

  if (stage == STAGE_PRINT) {
    decl_print(parser_result, 0);
    printf("\n");
    return EXIT_SUCCESS;
  }

  resolver_trace = stage == STAGE_RESOLVE;
  scope_enter();
  decl_resolve(parser_result);
  scope_exit();
  if (resolver_error_count()) return EXIT_COMPILE_ERROR;
  if (stage == STAGE_RESOLVE) return EXIT_SUCCESS;

  decl_typecheck(parser_result);
  if (typecheck_error_count()) return EXIT_COMPILE_ERROR;

  // Check AST invariant: "all types are concrete and all names are bound"
  // NOTE: Array/initializer containers are exempt from value-type requirements
  if (!decl_validate(parser_result)) {
    typecheck_error("internal: typed AST invariant failed");
    return EXIT_COMPILE_ERROR;
  }

  if (stage == STAGE_CODEGEN) {
    fprintf(stderr, "bminor: stage not implemented (%s)\n", flag);
    return EXIT_COMPILE_ERROR;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  // stdout set to line-buffer (vs. full-buffer):
  // - Allows real-time processing of output
  //   - Minimizes interleaving from multithreading (once implemented)
  //   - Better for piping to other programs (according to google)
  if (setvbuf(stdout, NULL, _IOLBF, 0) != 0) {
    fprintf(stderr, "bminor: failed to configure output buffering\n");
    return EXIT_COMPILE_ERROR;
  }

  if (argc != 3) {
    print_usage(argv[0]);
    return EXIT_USAGE_ERROR;
  }

  stage_t stage = stage_from_flag(argv[1]);
  if (stage == STAGE_INVALID) {
    fprintf(stderr, "bminor: invalid flag (%s)\n", argv[1]);
    print_usage(argv[0]);
    return EXIT_USAGE_ERROR;
  }

  FILE *input = fopen(argv[2], "r");
  if (!input) {
    fprintf(stderr, "bminor: failed to open '%s': ", argv[2]);
    perror(NULL);
    return EXIT_COMPILE_ERROR;
  }

  scanner_open(input);
  int status = compile(stage, argv[1]);
  scanner_close();

  if (fclose(input) != 0) {
    perror("bminor: failed to close input");
    status = EXIT_COMPILE_ERROR;
  }

  memory_release();
  return status;
}
