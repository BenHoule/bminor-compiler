#pragma once

#include "type.h"

#include <stdint.h>
#include <stdio.h>

#define IR_NONE (-1)

typedef enum {
  IR_CONST,   // dst = imm.i
  IR_DCONST,  // dst = imm.d
  IR_STRING,  // dst = imm.s
  IR_COPY,    // dst = arg1

  // arithmetic
  IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD, IR_POW, IR_NEG,

  // floating-point arithmetic (no mod/pow)
  IR_FADD, IR_FSUB, IR_FMUL, IR_FDIV, IR_FNEG,

  // comparison
  IR_EQ, IR_NE, IR_LT, IR_LE, IR_GT, IR_GE,

  // floating-point comparison
  IR_FEQ, IR_FNE, IR_FLT, IR_FLE, IR_FGT, IR_FGE,

  IR_NOT,
  IR_PRINT,  // print arg1 as imm.kind

  // Memory
  IR_GLOBAL,  // dst = address of global imm.sym
  IR_ARRAY,   // dst = new zeroed array; args = lengths, outermost first
  IR_INDEX,   // dst = address of arg1[arg2]; bounds-checked when imm.i != 0
  IR_LOAD,    // dst = *arg1
  IR_STORE,   // *arg1 = arg2
  IR_LEN,     // dst = length of array at arg1

  IR_CALL,    // dst = imm.sym(args...); IR_NONE for void

  // Terminators (exactly one per block)
  IR_JUMP,   // jump to imm.target[0]
  IR_BRANCH, // if arg1 goto imm.target[0] else goto imm.target[1]
  IR_RETURN  // return arg1 (IR_NONE for void)
} ir_op_t;

// Forward declarations
struct ir_block;
struct symbol;

struct ir_instr {
  ir_op_t op;
  int dst, arg1, arg2;  // Virtual Registers (IR_NONE when unused)
  int *args, nargs;     // IR_CALL arguments / IR_ARRAY lengths

  union {
    int64_t i;
    double d;
    const char *s;
    type_t kind;
    struct ir_block *target[2];
    const struct symbol *sym;  // For IR_GLOBAL and IR_CALL
  } imm;

  struct ir_instr *next;
};

struct ir_block {
  int id;
  struct ir_instr *head, *tail;  // tail is terminator
  struct ir_block *next;
};

struct ir_func {
  const char *name;
  int nparams;  // %0 .. %p-1
  int nlocals;  // %p .. %l-1
  int nregs;    // %l .. %r-1  (temp registers)
  int nblocks;

  struct ir_block *head, *tail;
  struct ir_block *current;
  struct ir_func *next;
};

/**
 * @brief Create a new IR function with the given properties.
 *
 * @param name The name of the function.
 * @param nparams The number of parameters.
 * @param nlocals The number of local variables.
 *
 * @return A pointer to the newly created IR function.
 */
struct ir_func *ir_func_create(const char *name, int nparams, int nlocals);

/**
 * @brief Create a new IR block and append it to the given function.
 *
 * @param func The IR function.
 *
 * @return A pointer to the newly created IR block.
 */
struct ir_block *ir_block_create(struct ir_func *func);

/**
 * @brief Allocate a new virtual register for the given IR function.
 *
 * @param func The IR function.
 *
 * @return The index of the newly allocated register.
 */
int ir_new_reg(struct ir_func *func);

/**
 * @brief Check if the given IR operation is a terminator.
 *
 * @param op The IR operation.
 *
 * @return true if the operation is a terminator, false otherwise.
 */
bool ir_is_terminator(ir_op_t op);

/**
 * @brief Emit a new IR instruction for the given function.
 *
 * @param func The IR function.
 * @param op The IR operation.
 * @param dst The destination register.
 * @param arg1 The first argument register.
 * @param arg2 The second argument register.
 *
 * @return A pointer to the newly created IR instruction.
 */
struct ir_instr *ir_emit(struct ir_func *func, ir_op_t op, int dst, int arg1, int arg2);

/**
 * @brief Emit a new IR instruction with a list of argument registers.
 *
 * @param func The IR function.
 * @param op The IR operation.
 * @param dst The destination register.
 * @param args The array of argument registers.
 * @param nargs The number of argument registers.
 *
 * @return A pointer to the newly created IR instruction.
 */
struct ir_instr *ir_emit_list(struct ir_func *func, ir_op_t op, int dst, int *args, int nargs);

/**
 * @brief Emit a global variable instruction.
 *
 * @param func The IR function.
 * @param global The global symbol.
 *
 * @return The destination register holding the global variable.
 */
int ir_emit_global(struct ir_func *func, const struct symbol *global);

/**
 * @brief Emit a new IR instruction with an automatically allocated destination register.
 *
 * @param func The IR function.
 * @param op The IR operation.
 * @param arg1 The first argument register.
 * @param arg2 The second argument register.
 *
 * @return The index of the destination register.
 */
int ir_emit_op(struct ir_func *func, ir_op_t op, int arg1, int arg2);

/**
 * @brief Emit a jump instruction to the given target block.
 *
 * @param func The IR function.
 * @param target The target block to jump to.
 */
void ir_emit_jump(struct ir_func *func, struct ir_block *target);

/**
 * @brief Emit a conditional branch instruction.
 *
 * @param func The IR function.
 * @param cond The condition register.
 * @param true_target The target block if the condition is true.
 * @param false_target The target block if the condition is false.
 */
void ir_emit_branch(struct ir_func *func, int cond, struct ir_block *true_target, struct ir_block *false_target);

/**
 * @brief Get the successor blocks of the given IR block.
 *
 * @param block The IR block.
 * @param out An array to store the successor blocks.
 *
 * @return The number of successor blocks.
 */
int ir_block_successors(const struct ir_block *block, struct ir_block *out[2]);

/**
 * @brief Place the given IR block in the function, ensuring proper control flow.
 *
 * @param func The IR function.
 * @param block The IR block to place.
 */
void ir_block_place(struct ir_func *func, struct ir_block *block);

struct ir_program {
  struct ir_func *init;    // Stores global values (runs before main)
  struct ir_func *funcs;   // Defined functions (in source order)
  int nglobals;            // Number of global variables
};

/**
 * @brief Print the IR function to the given output stream.
 *
 * @param out The output stream.
 * @param prog The IR program to print.
 */
void ir_print(FILE *out, const struct ir_program *prog);


