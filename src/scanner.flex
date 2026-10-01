%option noyywrap nounput noinput nodefault warn
%{

#include "scanner.h"
#include "bminor.tab.h"
#include "string_util.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static int line = 1;
static int col = 1;
/* Macro executed automatically before every token action */
#define YY_USER_ACTION \
  yylloc.first_line   = line; \
  yylloc.first_column = col; \
  yylloc.last_line     = line; \
  yylloc.last_column   = col + yyleng - 1; \
  for (int i = 0; i < yyleng; i++) { \
    if (yytext[i] == '\n') { \
      line++; \
      col = 1; \
    } else { \
      col++; \
    } \
  }

static int err_count = 0;
static int block_start_line = 1;
static int block_start_col = 1;

/* Tells compiler to check printf-style format and arguments for scanner_error */
[[gnu::format(printf, 1, 2)]]
static void scanner_error(const char *msg, ...);
static void send_integer_to_yylval(const uint8_t base);
static void send_double_to_yylval();

%}

%x COMMENT_BLOCK

  /* DEFINITIONS */
D   [0-9]
NZ  [1-9]
H   [a-fA-F0-9]
B   [01]
L   [a-zA-Z_]
A   [a-zA-Z_0-9]
E   ([eE][+-]?{D}+)
ES  (\\(0x{H}{H}|[^\n]))
C  ([^\'\\\n]|{ES})
S  ([^\"\\\n]|{ES})
NUL  \\0x00
WHITESPACE  [ \t\r\n\f]
IDENTIFIER  {L}{A}*

%%
  /* RULES */
"//"[^\n]*  { /* Single-line comments */ }
"/*"        { // Multi-line comments
              BEGIN(COMMENT_BLOCK);
              block_start_line = yylloc.first_line;
              block_start_col = yylloc.first_column;
            }

<COMMENT_BLOCK>{
  "*/"      { BEGIN(INITIAL); }

  [^*]+     { /* Ignore everything else inside comment */ }
  "*"       { /* Makes scanner check if next char is '/' */ }

  <<EOF>>   {
              err_count++;
              fprintf(stderr, "line %d, col %d: Unterminated block comment\n",
                      block_start_line, block_start_col);
              BEGIN(INITIAL);
              yyterminate();
            }
}

{WHITESPACE}+  { /* Ignore whitespace */ }

"array"       { return TOKEN_ARRAY; }
"auto"        { return TOKEN_AUTO; }
"boolean"     { return TOKEN_BOOLEAN; }
"carray"      { return TOKEN_CARRAY; }
"char"        { return TOKEN_CHAR; }
"double"      { return TOKEN_DOUBLE; }
"else"        { return TOKEN_ELSE; }
"false"       { return TOKEN_FALSE; }
"float"       { return TOKEN_FLOAT; }
"for"         { return TOKEN_FOR; }
"function"    { return TOKEN_FUNCTION; }
"if"          { return TOKEN_IF; }
"integer"     { return TOKEN_INTEGER; }
"print"       { return TOKEN_PRINT; }
"return"      { return TOKEN_RETURN; }
"string"      { return TOKEN_STRING; }
"true"        { return TOKEN_TRUE; }
"void"        { return TOKEN_VOID; }
"while"       { return TOKEN_WHILE; }

"++"          { return TOKEN_INC; }
"--"          { return TOKEN_DEC; }
"&&"          { return TOKEN_AND; }
"||"          { return TOKEN_OR; }
"=="          { return TOKEN_EQ; }
">="          { return TOKEN_GE; }
"<="          { return TOKEN_LE; }
"!="          { return TOKEN_NE; }

"0x"{H}+  { /* INTEGER_LITERAL*/
            static constexpr uint8_t BASE_16 = 16;
            send_integer_to_yylval(BASE_16);
            return TOKEN_INTEGER_LITERAL;
          }
"0b"{B}+  {
            static constexpr uint8_t BASE_2 = 2;
            send_integer_to_yylval(BASE_2);
            return TOKEN_INTEGER_LITERAL;
          }
{D}+      {
            static constexpr uint8_t BASE_10 = 10;
            send_integer_to_yylval(BASE_10);
            return TOKEN_INTEGER_LITERAL;
          }

{D}+{E}           { /* DOUBLE_LITERAL */
                    send_double_to_yylval();
                    return TOKEN_DOUBLE_LITERAL;
                  }
{D}*"."{D}+{E}?   {
                    send_double_to_yylval();
                    return TOKEN_DOUBLE_LITERAL;
                  }
{D}+"."{E}?       {
                    send_double_to_yylval();
                    return TOKEN_DOUBLE_LITERAL;
                  }

\"{S}*{NUL}{S}*\"   { /* STRING_LITERAL
                       * NOTE:There are likely cleaner ways to handle this case */
                      yylval.sval = NULL;
                      scanner_error("String contains null character");
                      return TOKEN_STRING_LITERAL;
                    }
\"{S}*\"            {
                      yylval.sval = NULL;

                      // Worst-Case: '\0xHH' repeated 255 times + 2 quotes
                      static constexpr size_t STRING_MAX = 255;
                      if (yyleng > ((STRING_MAX * 5) + 2)) {
                        scanner_error("String literal too long (MAX=255)");
                        return TOKEN_STRING_LITERAL;
                      }
                      uint8_t encoded_str[STRING_MAX + 1] = {0};
                      switch (string_encode(yytext, encoded_str, STRING_MAX + 1)) {
                        case ERR_INVALID_CHAR:
                          scanner_error("String literal contains character outside of ASCII range [0-127]");
                          break;
                        case ERR_TOO_LONG:
                          scanner_error("String literal too long (MAX=255)");
                          return TOKEN_STRING_LITERAL;
                        default: break;
                      }

                      yylval.sval = strdup((const char *)encoded_str);
                      if (yylval.sval == NULL)
                        scanner_error("Failed to allocate memory for string literal");

                      return TOKEN_STRING_LITERAL;
                    }
\"{S}*              {
                      yylval.sval = NULL;
                      scanner_error("Unterminated string");
                      return TOKEN_STRING_LITERAL;
                    }

'{C}'       { /* CHAR_LITERAL
               * NOTE: `char_encode` may return a value outside of
               * ASCII range [0-127], though Regex should ensure
               * yytext is a valid single character.
               */
              const unsigned char *ctext = (const unsigned char *)(yytext + 1);  // Skip the opening quote
              const uint8_t byte = char_encode(&ctext);
              if (byte > 127)
                scanner_error("Char outside of ASCII range [0-127]");
              yylval.cval = (char)byte;

              return TOKEN_CHAR_LITERAL;
            }
'{C}{2,}'   { scanner_error("Char literal too long (MAX=1)"); return TOKEN_CHAR_LITERAL;}
'{C}*       { scanner_error("Unterminated char"); return TOKEN_CHAR_LITERAL; }
''          { scanner_error("Empty char literal (use '\\0x00' for null)"); return TOKEN_CHAR_LITERAL; }

{IDENTIFIER}  { // Last so it loses ties
  yylval.sval = NULL;
  static constexpr size_t IDENTIFIER_MAX = 255;
  if (yyleng > IDENTIFIER_MAX) {
    scanner_error("Identifier too long (MAX=255)");
    return TOKEN_IDENTIFIER;
  }

  yylval.sval = strdup(yytext);
  if (yylval.sval == NULL)
    scanner_error("Failed to allocate memory for identifier");

  // If B-Minor included typedefs or enums, we would check for those here
  return TOKEN_IDENTIFIER;
}

";"           { return ';'; }
":"           { return ':'; }
","           { return ','; }
"("           { return '('; }
")"           { return ')'; }
"["           { return '['; }
"]"           { return ']'; }
"{"           { return '{'; }  /* Spec kind of glosses over braces */
"}"           { return '}'; }
"#"           { return '#'; }
"!"           { return '!'; }
"^"           { return '^'; }
"*"           { return '*'; }
"/"           { return '/'; }
"%"           { return '%'; }
"+"           { return '+'; }
"-"           { return '-'; }
">"           { return '>'; }
"<"           { return '<'; }
"="           { return '='; }

.   {
      scanner_error("Unrecognized character '%c' (0x%02X)",
                    isprint((unsigned char)yytext[0]) ? yytext[0] : '?',
                    (unsigned char)yytext[0]);
    }
%%
  /* HELPERS */
unsigned int scanner_error_count(void) {
  return err_count;
}

static void scanner_error(const char *fmt, ...) {
  err_count++;
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "line %d, col %d: ", yylloc.first_line, yylloc.first_column);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}

static void send_integer_to_yylval(const uint8_t base) {
  errno = 0;
  yylval.ival = strtoll(yytext, NULL, base);
  if (errno == ERANGE) scanner_error("Integer literal out of range (-2e63, 2e63-1)");
}

static void send_double_to_yylval() {
  errno = 0;
  yylval.dval = strtod(yytext, NULL);
  if (errno == ERANGE) scanner_error("Double literal out of range (+-1.7e-308, +-1.7e308)");
}

void scanner_open(FILE *file) {
  yyrestart(file);
}

int scanner_next_token(void) {
  return yylex();
}



