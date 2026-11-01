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
  IR_ADD, IR_SUB, IR_MUL, IR_DIV,
  IR_MOD, IR_POW, IR_NEG,

  // floating-point arithmetic
  IR_FADD, IR_FSUB, IR_FMUL, IR_FDIV,
  IR_FPOW, IR_FNEG,

  // comparison
  IR_EQ, IR_NE, IR_LT, IR_LE, IR_GT, IR_GE,

  // floating-point comparison
  IR_FEQ, IR_FNE, IR_FLT, IR_FLE, IR_FGT, IR_FGE,

  IR_NOT,
  IR_PRINT,  // print arg1 as imm.kind
  IR_RETURN  // return arg1 (IR_NONE for void)
} ir_op_t;

struct ir_instr {
  ir_op_t op;
  int dst, arg1, arg2;  // Virtual Registers (IR_NONE when unused)

  union {
    int64_t i;
    double d;
    bool b;
    char c;
    const char *s;
    type_t kind;
  } imm;

  struct ir_instr *next;
};

struct ir_func {
  const char *name;
  int nparams;  // %0 .. %p-1
  int nlocals;  // %p .. %l-1
  int nregs;    // %l .. %r-1  (temp registers)

  struct ir_instr *head, *tail;
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
 * @brief Allocate a new virtual register for the given IR function.
 *
 * @param func The IR function.
 *
 * @return The index of the newly allocated register.
 */
int ir_new_reg(struct ir_func *func);

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
 * @brief Print the IR function to the given output stream.
 *
 * @param out The output stream.
 * @param func The IR function to print.
 */
void ir_print(FILE *out, const struct ir_func *func);
