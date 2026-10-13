#include "scanner.h"
#include "bminor.tab.h"
#include "bminor_parser.h"
#include "decl.h"

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

// Lookup table: command-line flags -> compilation stages
static stage_t stage_from_flag(const char *user_flag) {
  struct {
    const char *flag;
    stage_t stage;
  } flag_table[] = {
    {"-scan",      STAGE_SCAN},
    {"-parse",     STAGE_PARSE},
    {"-print",     STAGE_PRINT},
    {"-resolve",   STAGE_RESOLVE},
    {"-typecheck", STAGE_TYPECHECK},
    {"-codegen",   STAGE_CODEGEN}
  };

  for (size_t i = 0; i < sizeof(flag_table) / sizeof(flag_table[0]); i++) {
    // Include null terminator in strncmp so that exact match is required
    const size_t flag_length = strlen(flag_table[i].flag) + 1;
    if (strncmp(user_flag, flag_table[i].flag, flag_length) == 0) {
      return flag_table[i].stage;
    }
  }

  return STAGE_INVALID;
}

static inline void print_usage(const char *prog_name) {
  fprintf(stderr, "Usage: %s <stage> <input file>\n", prog_name);
}

// - `argv[1]` = stage flag
// - `argv[2]` = input file
int main(int argc, char *argv[]) {
  if (argc != 3) {
    print_usage(argv[0]);
    return EXIT_USAGE_ERROR;
  }

  const stage_t stage = stage_from_flag(argv[1]);

  FILE *input_file = fopen(argv[2], "r");
  if (input_file == NULL) {
    char err_msg[256];
    snprintf(err_msg, sizeof(err_msg),
      "bminor: failed to open '%s'", argv[2]);
    perror(err_msg);
    return EXIT_COMPILE_ERROR;
  }
  scanner_open(input_file);

  switch (stage) {
    case STAGE_SCAN:
      int token = 0;
      while((token = scanner_next_token()) != 0) {
        const char *name = token_name(token);
        printf("%s\n", name);
        if (token == TOKEN_STRING_LITERAL || token == TOKEN_IDENTIFIER) {
          free(yylval.sval);
        }
      }
      break;
    case STAGE_PARSE:
      yyparse(); // Parsing logic would go here
      break;
    case STAGE_PRINT:
      yyparse();
      if (parser_error_count() > 0) {
        fclose(input_file);
        return EXIT_COMPILE_ERROR;
      }
      decl_print(parser_result, 0);
      printf("\n");
      break;
    case STAGE_RESOLVE:
    case STAGE_TYPECHECK:
    case STAGE_CODEGEN:
      fprintf(stderr, "bminor: stage not implemented (%s)\n", argv[1]);
      fclose(input_file);
      return EXIT_COMPILE_ERROR;
    default:
      fprintf(stderr, "bminor: invalid flag (%s)\n", argv[1]);
      print_usage(argv[0]);
      fclose(input_file);
      return EXIT_USAGE_ERROR;
  }
  fclose(input_file);

  unsigned int total_err = scanner_error_count() + parser_error_count();
  return total_err > 0 ? EXIT_COMPILE_ERROR : EXIT_SUCCESS;
}
