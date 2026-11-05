#pragma once

#include <stdio.h>

struct ir_program; // Forward declaration

void codegen_program(FILE *out, const struct ir_program *program);
