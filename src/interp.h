#pragma once

struct ir_func;  // Forward declaration

/**
 * Runs `main` from the given IR program.
 *
 * @param program The IR program to execute.
 *
 * @return The program's exit status.
 */
int interp_run(const struct ir_func *program);
