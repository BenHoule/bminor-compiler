#pragma once

#include <stdio.h>

void scanner_open(FILE *);
void scanner_set_error_prefix(const char *prefix);

int scanner_next_token(void);

unsigned int scanner_error_count(void);
