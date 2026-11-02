#include "ir.h"
#include "memory.h"

#include <inttypes.h>
#include <stdlib.h>

struct ir_func *ir_func_create(const char *name, int nparams, int nlocals) {
  struct ir_func *func = memory_allocate(sizeof *func);
  if (!func) {
    fprintf(stderr, "Failed to allocate memory for ir_func\n");
    exit(EXIT_FAILURE);
  }
  func->name = name;
  func->nparams = nparams;
  func->nlocals = nlocals;
  func->nregs = nparams + nlocals;
  func->head = func->tail = nullptr;
  func->next = nullptr;
  return func;
}

int ir_new_reg(struct ir_func *func) {
  return func->nregs++;
}

struct ir_instr *ir_emit(struct ir_func *func, ir_op_t op, int dst, int arg1, int arg2) {
  struct ir_instr *instr = memory_allocate(sizeof *instr);
  if (!instr) {
    fprintf(stderr, "Failed to allocate memory for ir_instr\n");
    exit(EXIT_FAILURE);
  }
  instr->op = op;
  instr->dst = dst;
  instr->arg1 = arg1;
  instr->arg2 = arg2;
  instr->imm.i = 0;
  instr->next = nullptr;

  if (func->tail) func->tail->next = instr;
  else func->head = instr;

  func->tail = instr;
  return instr;
}

int ir_emit_op(struct ir_func *func, ir_op_t op, int arg1, int arg2) {
  int dst = ir_new_reg(func);
  ir_emit(func, op, dst, arg1, arg2);
  return dst;
}

static const char * ir_op_name( ir_op_t op ) {
  static const char *names[] = {
    [IR_CONST]  = "const",  [IR_DCONST] = "dconst",
    [IR_STRING] = "string", [IR_COPY]   = "copy",

    [IR_ADD] = "add", [IR_SUB] = "sub",
    [IR_MUL] = "mul", [IR_DIV] = "div",
    [IR_MOD] = "mod", [IR_POW] = "pow",
    [IR_NEG] = "neg",

    [IR_FADD] = "fadd", [IR_FSUB] = "fsub",
    [IR_FMUL] = "fmul", [IR_FDIV] = "fdiv",
    [IR_FNEG] = "fneg",

    [IR_EQ] = "eq", [IR_NE] = "ne",
    [IR_LT] = "lt", [IR_LE] = "le",
    [IR_GT] = "gt", [IR_GE] = "ge",

    [IR_FEQ] = "feq", [IR_FNE] = "fne",
    [IR_FLT] = "flt", [IR_FLE] = "fle",
    [IR_FGT] = "fgt", [IR_FGE] = "fge",

    [IR_NOT] = "not", [IR_PRINT] = "print", [IR_RETURN] = "ret",
  };
  return names[op];
}

static const char * ir_kind_name( type_t kind ) {
  switch (kind) {
    case TYPE_BOOLEAN: return "boolean";
    case TYPE_CHAR:    return "char";
    case TYPE_INTEGER: return "integer";
    case TYPE_DOUBLE:  return "double";
    case TYPE_STRING:  return "string";
    default:           return "?";
  }
}

void ir_print(FILE *out, const struct ir_func *func) {
  for (; func; func = func->next) {
    fprintf(out, "function %s (params=%d, locals=%d, regs=%d)\n",
            func->name, func->nparams, func->nlocals, func->nregs);

    for (const struct ir_instr *instr = func->head; instr; instr = instr->next) {
      fprintf(out, "  ");
      if (instr->dst != IR_NONE) fprintf(out, "%%%d = ", instr->dst);
      fprintf(out, "%s", ir_op_name(instr->op));

      switch (instr->op) {
        case IR_CONST:  fprintf(out, " %" PRId64, instr->imm.i); break;
        case IR_DCONST: fprintf(out, " %.17g", instr->imm.d); break;
        case IR_STRING: fprintf(out, " <string>"); break;
        case IR_PRINT:  fprintf(out, ".%s %%%d",
                                ir_kind_name(instr->imm.kind), instr->arg1); break;
        default:
          if (instr->arg1 != IR_NONE) fprintf(out, " %%%d", instr->arg1);
          if (instr->arg2 != IR_NONE) fprintf(out, ", %%%d", instr->arg2);
      }
      fprintf(out, "\n");
    }
    fprintf(out, "\n");
  }
}
