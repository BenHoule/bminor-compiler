#pragma once

struct ir_program;  // Forward declaration

/**
 * @brief Runs the IR optimization passes over every function in the program.
 *
 * Rewrites the IR in place. Observable behaviour (output, exit status, runtime
 * errors) must be identical to the unoptimized program.
 */
void optimize_program(struct ir_program *prog);
