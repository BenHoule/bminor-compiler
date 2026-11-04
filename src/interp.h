#pragma once

// Forward declarations
struct ir_func;
struct ir_program;

/**
 * Runs `main` from the given IR program.
 *
 * @param program The IR program to execute.
 *
 * @return The program's exit status.
 */
int interp_run(const struct ir_program *program);
