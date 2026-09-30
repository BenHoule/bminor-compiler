%option noyywrap nounput noinput yylineno
%option nodefault warn
%{

#include "scanner.h"
#include "parser.h"
#include "string_util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

static int err_count = 0;
static int block_start_line = 0;

static void scanner_error(const char *msg);
static void send_integer_to_yylval(const uint8_t base);
static void send_double_to_yylval();


void scanner_open(FILE *file);
int scanner_next_token(void);
void get_token_name(char *buf, int token, size_t buf_size);

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
              // NOTE: Need to change too yylloc if using location tracking
              BEGIN(COMMENT_BLOCK);
              block_start_line = yylineno;
            }

<COMMENT_BLOCK>{
  "*/"      { BEGIN(INITIAL); }

  [^*]+     { /* Ignore everything else inside comment */ }
  "*"       { /* Makes scanner check if next char is '/' */ }

  <<EOF>>   {
              const int block_end_line = yylineno;
              yylineno = block_start_line;
              scanner_error("Unterminated block comment");
              yylineno = block_end_line;
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

                      static constexpr size_t ERR_INVALID_CHAR = 0;
                      static constexpr size_t ERR_TOO_LONG = SIZE_MAX;
                      switch (string_encode(yytext, encoded_str, STRING_MAX + 1)) {
                        case ERR_INVALID_CHAR:
                          scanner_error("String literal contains character outside of ASCII range [0-127]");
                          break;
                        case ERR_TOO_LONG:
                          scanner_error("String literal too long (MAX=255)");
                          return TOKEN_STRING_LITERAL;
                        default: break;
                      }

                      yylval.sval = strdup(encoded_str);
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
              yylval.cval = char_encode(yytext + 1);  // Skip opening quote
              if (yylval.cval > 127)
                scanner_error("Char outside of ASCII range [0-127]");

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

.   { scanner_error("Unrecognized character"); }

%%
  /* HELPERS */
unsigned int scanner_error_count(void) {
  return err_count;
}

static void scanner_error(const char *msg) {
  err_count++;
  fprintf(stderr, "line %d: %s\n", yylineno, msg);
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

void get_token_name(char *buf, int token, size_t buf_size) {
  const char *name = NULL;

  switch(token) {
    case TOKEN_IDENTIFIER: name = "TOKEN_IDENTIFIER"; break;
    case TOKEN_BOOLEAN: name = "TOKEN_BOOLEAN"; break;
    case TOKEN_CHAR: name = "TOKEN_CHAR"; break;
    case TOKEN_DOUBLE: name = "TOKEN_DOUBLE"; break;
    case TOKEN_FLOAT: name = "TOKEN_FLOAT"; break;
    case TOKEN_INTEGER: name = "TOKEN_INTEGER"; break;
    case TOKEN_STRING: name = "TOKEN_STRING"; break;
    case TOKEN_ARRAY: name = "TOKEN_ARRAY"; break;
    case TOKEN_AUTO: name = "TOKEN_AUTO"; break;
    case TOKEN_CARRAY: name = "TOKEN_CARRAY"; break;
    case TOKEN_FUNCTION: name = "TOKEN_FUNCTION"; break;
    case TOKEN_VOID: name = "TOKEN_VOID"; break;
    case TOKEN_CHAR_LITERAL: name = "TOKEN_CHAR_LITERAL"; break;
    case TOKEN_DOUBLE_LITERAL: name = "TOKEN_DOUBLE_LITERAL"; break;
    case TOKEN_INTEGER_LITERAL: name = "TOKEN_INTEGER_LITERAL"; break;
    case TOKEN_STRING_LITERAL: name = "TOKEN_STRING_LITERAL"; break;
    case TOKEN_FALSE: name = "TOKEN_FALSE"; break;
    case TOKEN_TRUE: name = "TOKEN_TRUE"; break;
    case TOKEN_ELSE: name = "TOKEN_ELSE"; break;
    case TOKEN_FOR: name = "TOKEN_FOR"; break;
    case TOKEN_IF: name = "TOKEN_IF"; break;
    case TOKEN_RETURN: name = "TOKEN_RETURN"; break;
    case TOKEN_WHILE: name = "TOKEN_WHILE"; break;
    case TOKEN_INC: name = "TOKEN_INC"; break;
    case TOKEN_DEC: name = "TOKEN_DEC"; break;
    case TOKEN_AND: name = "TOKEN_AND"; break;
    case TOKEN_OR: name = "TOKEN_OR"; break;
    case TOKEN_EQ: name = "TOKEN_EQ"; break;
    case TOKEN_GE: name = "TOKEN_GE"; break;
    case TOKEN_LE: name = "TOKEN_LE"; break;
    case TOKEN_NE: name = "TOKEN_NE"; break;
    case TOKEN_PRINT: name = "TOKEN_PRINT"; break;
    default:
      if (token < 0 || 127 < token) {
        name = "UNKNOWN_TOKEN";
        break;
      }
      buf[0] = (char)token;
      buf[1] = '\0';
      break;
  }
  if (name != NULL) {
    strncpy(buf, name, buf_size - 1);
    buf[buf_size - 1] = '\0';
  }
}

