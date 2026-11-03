#include "interp.h"
#include "ir.h"
#include "library.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

union value {
  int64_t i;
  double d;
  const char *s;
  // bool b; (?)
};

static void interp_print(type_t kind, union value value) {
  switch (kind) {
    case TYPE_BOOLEAN: print_boolean((bool)value.i); break;
    case TYPE_CHAR:    print_character((char)value.i); break;
    case TYPE_DOUBLE:  print_double(value.d); break;
    case TYPE_INTEGER: print_integer(value.i); break;
    case TYPE_STRING:  print_string(value.s); break;

    default: abort();
  }
}

static union value interp_call(const struct ir_func *func) {
  union value *r = calloc(func->nregs ? func->nregs : 1, sizeof *r);
  if (!r) {
    fprintf(stderr, "interp error: failed to allocate memory for registers\n");
    exit(EXIT_FAILURE);
  }

  union value result = {0};
  const struct ir_block *block = func->head;
  for (;;) {
    const struct ir_block *next = nullptr;

    for (const struct ir_instr *instr = block->head; instr; instr = instr->next) {
      union value arg1 = instr->arg1 != IR_NONE ? r[instr->arg1] : (union value){0};
      union value arg2 = instr->arg2 != IR_NONE ? r[instr->arg2] : (union value){0};
      union value *dst = instr->dst != IR_NONE ? &r[instr->dst] : nullptr;

      switch (instr->op) {
        case IR_CONST:  dst->i = instr->imm.i; break;
        case IR_DCONST: dst->d = instr->imm.d; break;
        case IR_STRING: dst->s = instr->imm.s; break;
        case IR_COPY:   *dst = arg1; break;

        case IR_ADD:  dst->i = arg1.i + arg2.i; break;
        case IR_SUB:  dst->i = arg1.i - arg2.i; break;
        case IR_MUL:  dst->i = arg1.i * arg2.i; break;
        case IR_DIV:
        case IR_MOD:
          if (arg2.i == 0) {
            fprintf(stderr, "runtime error: division by zero\n");
            exit(EXIT_FAILURE);
          }
          dst->i = instr->op == IR_DIV ? arg1.i / arg2.i : arg1.i % arg2.i;
          break;
        case IR_POW:  dst->i = integer_power(arg1.i, arg2.i); break;
        case IR_NEG:  dst->i = -arg1.i; break;

        case IR_FADD: dst->d = arg1.d + arg2.d; break;
        case IR_FSUB: dst->d = arg1.d - arg2.d; break;
        case IR_FMUL: dst->d = arg1.d * arg2.d; break;
        case IR_FDIV: dst->d = arg1.d / arg2.d; break;  // **IEEE 754** (-)fp / 0.0 = (-)INFINITY; 0.0 / 0.0 = NaN
        case IR_FNEG: dst->d = -arg1.d; break;

        case IR_EQ:  dst->i = arg1.i == arg2.i; break;
        case IR_NE:  dst->i = arg1.i != arg2.i; break;
        case IR_LT:  dst->i = arg1.i <  arg2.i; break;
        case IR_LE:  dst->i = arg1.i <= arg2.i; break;
        case IR_GT:  dst->i = arg1.i >  arg2.i; break;
        case IR_GE:  dst->i = arg1.i >= arg2.i; break;

        case IR_FEQ: dst->i = arg1.d == arg2.d; break;
        case IR_FNE: dst->i = arg1.d != arg2.d; break;
        case IR_FLT: dst->i = arg1.d <  arg2.d; break;
        case IR_FLE: dst->i = arg1.d <= arg2.d; break;
        case IR_FGT: dst->i = arg1.d >  arg2.d; break;
        case IR_FGE: dst->i = arg1.d >= arg2.d; break;

        case IR_NOT: dst->i = !arg1.i; break;

        case IR_PRINT: interp_print(instr->imm.kind, arg1); break;

        case IR_JUMP:   next = instr->imm.target[0]; break;
        case IR_BRANCH: next = instr->imm.target[arg1.i ? 0 : 1]; break;
        case IR_RETURN:
          result = arg1;
          free(r);
          return result;

        default: unreachable();
      }
    }

    if (!next) {
      fprintf(stderr, "interp error: block L%d has no terminator\n", block->id);
      abort();
    }
    block = next;
  }

  free(r);
  return result;
}

int interp_run(const struct ir_func *program) {
  for (const struct ir_func *func = program; func != NULL; func = func->next) {
    if (strcmp(func->name, "main") == 0) {
      int status = (int)(interp_call(func).i & 0xFF);
      fflush(stdout);
      return status;
    }
  }

  fprintf(stderr, "interp error: main function not found\n");
  return EXIT_FAILURE;
}
