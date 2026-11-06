#include "bminor.tab.h"
#include "bminor_parser.h"
#include "codegen.h"
#include "decl.h"
#include "interp.h"
#include "ir.h"
#include "memory.h"
#include "resolver.h"
#include "scanner.h"
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
  STAGE_VALIDATE,
  STAGE_IR,
  STAGE_INTERP,
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
    {"-scan",    STAGE_SCAN},
    {"-parse",   STAGE_PARSE},
    {"-print",   STAGE_PRINT},
    {"-resolve", STAGE_RESOLVE},
    {"-typecheck", STAGE_TYPECHECK},
    {"-validate",  STAGE_VALIDATE},
    {"-ir",      STAGE_IR},
    {"-interp",  STAGE_INTERP},
    {"-codegen", STAGE_CODEGEN},
  };
  for (size_t i = 0; i < sizeof flags / sizeof *flags; i++)
    if (strcmp(flag, flags[i].flag) == 0) return flags[i].stage;

  return STAGE_INVALID;
}

static void print_usage(const char *program) {
  fprintf(stderr, "Usage: %s <stage> <input file>\n"
          "Stages: -scan -parse -print -resolve -typecheck -validate\n"
          "        -ir -interp -codegen\n", program);
}

static int compile(stage_t stage) {
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
  if (stage == STAGE_TYPECHECK) return EXIT_SUCCESS;

  // AST invariant: "all types are concrete and all names are bound"
  // - Array/initializer containers are exempt from value-type requirements
  if (!decl_validate(parser_result)) {
    typecheck_error("internal: typed AST invariant failed");
    return EXIT_COMPILE_ERROR;
  }
  if (stage == STAGE_VALIDATE) return EXIT_SUCCESS;

  struct ir_program *program = decl_lower_program(parser_result);
  if (stage == STAGE_IR) {
    ir_print(stdout, program);
    return EXIT_SUCCESS;
  }

  // Symbolic execution of lowered program to catch runtime errors
  if (stage == STAGE_INTERP) return interp_run(program);

  if (stage == STAGE_CODEGEN) {
    codegen_program(stdout, program);
    return EXIT_SUCCESS;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  // stdout set to line-buffer (vs. full-buffer):
  // - Allows real-time processing of output
  //   - Minimizes interleaving from multithreading (once implemented)
  //   - Better for piping to other programs
  if (setvbuf(stdout, nullptr, _IOLBF, 0) != 0) {
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
    perror(nullptr);
    return EXIT_COMPILE_ERROR;
  }

  scanner_open(input);
  int status = compile(stage);
  scanner_close();

  if (fclose(input) != 0) {
    perror("bminor: failed to close input");
    status = EXIT_COMPILE_ERROR;
  }

  memory_release();
  return status;
}
