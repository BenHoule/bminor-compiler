%option noyywrap nounput noinput yylineno
%{

#include "scanner.h"
#include "parser.h"
#include "string_util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int err_count = 0;

static int yyerror(const char *msg);

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
"/*"        { BEGIN(COMMENT_BLOCK); /* Multi-line comments */ }

<COMMENT_BLOCK>{
  "*/"      { BEGIN(INITIAL); }

  /* NOTE: Pattern matching with ".*" matches one character at a time
  *  (slow for large comments). The method used below is more efficient.
  */
  [^*]+     { /* Ignore everything else inside comment */ }
  "*"       { /* Makes scanner check if next char is '/' */ }

  <<EOF>>   {
              yyerror("unterminated block comment");
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
                        static constexpr size_t STRING_MAX = 256;  //  255 + null terminator
                        char encoded_str[STRING_MAX] = {};
                        if (!string_encode(yytext, encoded_str, STRING_MAX))
                          yyerror("String literal too long (MAX=255)");
                        return TOKEN_STRING_LITERAL;
                      }
\"([^"\\\n]|{ES})*    { yyerror("Unterminated string"); }

'([^'\\\n]|{ES})*'    {
                        char encoded_char = '\0';
                        if (!char_encode(yytext, &encoded_char))
                          yyerror("Char literal too long (MAX=1)");
                        return TOKEN_CHAR_LITERAL;
                      }
'([^'\\\n]|{ES})*       { yyerror("Unterminated char"); }

{IDENTIFIER}  { /* Last so it loses ties */
  if (strlen(yytext) > 255) {
    yyerror("Identifier too long (MAX=255)");
  }
  /* If B-Minor included typedefs or enums, we would check for those here */
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

.   { yyerror("Unrecognized character"); }

%%
  /* HELPERS */
unsigned int scanner_error_count(void) {
  return err_count;
}

static int yyerror(const char *msg) {
  err_count++;
  fprintf(stderr, "line %d: %s\n", yylineno, msg);
  return 0;
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

