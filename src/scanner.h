#pragma once

#include <stdio.h>

void scanner_open(FILE *);

int scanner_next_token(void);

unsigned int scanner_error_count(void);
