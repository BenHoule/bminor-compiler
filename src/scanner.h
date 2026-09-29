#pragma once

#include <stdio.h>

void scanner_open(FILE *);

int scanner_next_token(void);

void get_token_name(char *buf, int token, size_t buf_size);

unsigned int scanner_error_count(void);
