%option noyywrap nounput noinput yylineno
%option nodefault warn
%{

#include "scanner.h"
#include "parser.h"
#include "string_util.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static int err_count = 0;

static int block_start_line = 0;

static void scanner_error(const char *msg);

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

"0x"{H}+         { return TOKEN_INTEGER_LITERAL; }
"0b"{B}+         { return TOKEN_INTEGER_LITERAL; }
{D}+             { return TOKEN_INTEGER_LITERAL; }

{D}+{E}          { return TOKEN_DOUBLE_LITERAL; }
{D}*"."{D}+{E}?  { return TOKEN_DOUBLE_LITERAL; }
{D}+"."{E}?      { return TOKEN_DOUBLE_LITERAL; }

\"([^"\\\n]|{ES})*\"  {
                        constexpr size_t STRING_MAX = 256;  // 255 + null terminator
                        uint8_t encoded_str[STRING_MAX] = {0};
                        yytext++;  // Skip the opening double quote

                        constexpr size_t ERR_INVALID_CHAR = 0;
                        constexpr size_t ERR_TOO_LONG = SIZE_MAX;
                        switch (string_encode(yytext, encoded_str, STRING_MAX)) {
                          case ERR_INVALID_CHAR:
                            scanner_error("String literal contains character outside of ASCII range [0-127]");
                            break;
                          case ERR_TOO_LONG:
                            scanner_error("String literal too long (MAX=255)");
                            break;
                          default: break;
                        }
                        return TOKEN_STRING_LITERAL;
                      }
\"([^"\\\n]|{ES})*    { scanner_error("Unterminated string"); }
'([^'\\\n]|{ES})'     {
                        uint8_t encoded_char = 0;
                        yytext++;  // Skip the opening single quote

                        const unsigned char *p = (const unsigned char *)yytext;
                        encoded_char = char_encode(&p);

                        // NOTE: `char_encode` may return a value outside of
                        // ASCII range [0-127]. Regex should ensure valid
                        // single character.
                        if (encoded_char > 127)
                          scanner_error("Char outside of ASCII range [0-127]");

                        return TOKEN_CHAR_LITERAL;
                      }
'([^'\\\n]|{ES}){2,}' { scanner_error("Char literal too long (MAX=1)"); }
'([^'\\\n]|{ES})*     { scanner_error("Unterminated char"); }
''                    { scanner_error("Empty char literal (use '\\0x00' for null)"); }

{IDENTIFIER}  { // Last so it loses ties
  if (yyleng > 255) {
    scanner_error("Identifier too long (MAX=255)");
  }
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

