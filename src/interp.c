#include "interp.h"
#include "ir.h"
#include "library.h"
#include "memory.h"
#include "symbol.h"

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
  union value *p;  // array block, or address of an element / global
};

// Arrays are blocks of length+1 values: [0] holds the length.
static const struct ir_program *interp_program;
static union value *interp_globals;

[[noreturn]] static void interp_fail(const char *msg) {
  fprintf(stderr, "runtime error: %s\n", msg);
  exit(EXIT_FAILURE);
}

static const struct ir_func *interp_find(const char *name) {
  for (const struct ir_func *func = interp_program->funcs; func; func = func->next)
    if (strcmp(func->name, name) == 0) return func;

  return nullptr;
}

static union value *interp_array(const int *dims, int ndims, const union value *r) {
  int64_t length = r[dims[0]].i;
  if (length <= 0) interp_fail("array length must be positive");
  if ((uint64_t)length >= SIZE_MAX / sizeof(union value)) interp_fail("array length too large");

  union value *array = memory_allocate((size_t)(length + 1) * sizeof *array);
  array[0].i = length;
  if (ndims > 1)
    for (int64_t i = 1; i <= length; i++)
      array[i].p = interp_array(dims + 1, ndims - 1, r);

  return array;
}

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

static union value interp_call(const struct ir_func *func, const int *args, const union value *caller) {
  union value *r = calloc(func->nregs ? func->nregs : 1, sizeof *r);
  if (!r) {
    fprintf(stderr, "interp error: failed to allocate memory for registers\n");
    exit(EXIT_FAILURE);
  }
  if (caller)
    for (int i = 0; i < func->nparams; i++) r[i] = caller[args[i]];

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

        // Conversion nonsense enables wrap-around on integer overflow (matches x86 behavior)
        case IR_ADD:  dst->i = (int64_t)((uint64_t)arg1.i + (uint64_t)arg2.i); break;
        case IR_SUB:  dst->i = (int64_t)((uint64_t)arg1.i - (uint64_t)arg2.i); break;
        case IR_MUL:  dst->i = (int64_t)((uint64_t)arg1.i * (uint64_t)arg2.i); break;
        case IR_DIV:
        case IR_MOD:
          if (arg2.i == 0) {
            fprintf(stderr, "runtime error: division by zero\n");
            exit(EXIT_FAILURE);
          }
          dst->i = instr->op == IR_DIV ? arg1.i / arg2.i : arg1.i % arg2.i;
          break;
        case IR_POW:  dst->i = integer_power(arg1.i, arg2.i); break;
        case IR_NEG:  dst->i = (int64_t)(0 - (uint64_t)arg1.i); break;

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

        case IR_GLOBAL: dst->p = &interp_globals[instr->imm.sym->which]; break;
        case IR_ARRAY:  dst->p = interp_array(instr->args, instr->nargs, r); break;
        case IR_INDEX:
          if (instr->imm.i && (arg2.i < 0 || arg2.i >= arg1.p[0].i))
            interp_fail("array index out of bounds");
          dst->p = &arg1.p[1 + arg2.i];
          break;

        case IR_LOAD:  *dst = *arg1.p; break;
        case IR_STORE: *arg1.p = arg2; break;
        case IR_LEN:  dst->i = arg1.p[0].i; break;

        case IR_CALL: {
          const struct ir_func *callee = interp_find(instr->imm.sym->name);
          if (!callee) interp_fail("call to a function with no body (external)");

          union value value = interp_call(callee, instr->args, r);
          if (dst) *dst = value;
          break;
        }

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

int interp_run(const struct ir_program *program) {
  interp_program = program;
  interp_globals = memory_allocate((size_t)(program->nglobals ? program->nglobals : 1) * sizeof *interp_globals);

  interp_call(program->init, nullptr, nullptr);

  const struct ir_func *main_func = interp_find("main");
  if (!main_func) {
    fprintf(stderr, "interp error: main function not found\n");
    return EXIT_FAILURE;
  }

  int status = (int)(interp_call(main_func, nullptr, nullptr).i & 0xFF);
  fflush(stdout);
  return status;
}
