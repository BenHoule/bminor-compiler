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
  ir_block_place(func, ir_block_create(func));
  return func;
}

struct ir_block *ir_block_create(struct ir_func *func) {
  struct ir_block *block = memory_allocate(sizeof *block);
  block->id = func->nblocks++;
  block->head = block->tail = nullptr;
  block->next = nullptr;
  return block;
}

int ir_new_reg(struct ir_func *func) {
  return func->nregs++;
}

bool ir_is_terminator(ir_op_t op) {
  return op == IR_JUMP || op == IR_BRANCH || op == IR_RETURN;
}

static bool ir_block_is_closed(const struct ir_block *block) {
  return block->tail && ir_is_terminator(block->tail->op);
}

static void ir_block_append(struct ir_func *func, struct ir_block *block) {
  if (func->tail) func->tail->next = block;
  else func->head = block;
  func->tail = block;
  func->current = block;
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

  if (ir_block_is_closed(func->current))
    ir_block_append(func, ir_block_create(func));

  struct ir_block *block = func->current;
  if (block->tail) block->tail->next = instr;
  else block->head = instr;

  block->tail = instr;
  return instr;
}

int ir_emit_op(struct ir_func *func, ir_op_t op, int arg1, int arg2) {
  int dst = ir_new_reg(func);
  ir_emit(func, op, dst, arg1, arg2);
  return dst;
}

void ir_emit_jump(struct ir_func *func, struct ir_block *target) {
  if (ir_block_is_closed(func->current)) return;
  ir_emit(func, IR_JUMP, IR_NONE, IR_NONE, IR_NONE)->imm.target[0] = target;
}

void ir_emit_branch(struct ir_func *func, int cond, struct ir_block *true_target, struct ir_block *false_target) {
  struct ir_instr *instr = ir_emit(func, IR_BRANCH, IR_NONE, cond, IR_NONE);
  instr->imm.target[0] = true_target;
  instr->imm.target[1] = false_target;
}

int ir_block_successors(const struct ir_block *block, struct ir_block *out[2]) {
  if (!block->tail) return 0;
  switch (block->tail->op) {
    case IR_JUMP:
      out[0] = block->tail->imm.target[0];
      return 1;
    case IR_BRANCH:
      out[0] = block->tail->imm.target[0];
      out[1] = block->tail->imm.target[1];
      return 2;
    default:
      return 0;
  }
}

void ir_block_place(struct ir_func *func, struct ir_block *block) {
  if (func->current && !ir_block_is_closed(func->current))
    ir_emit_jump(func, block);  // Prevents implicit fall-through

  ir_block_append(func, block);
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

    [IR_NOT] = "not",

    [IR_PRINT] = "print",

    [IR_JUMP] = "jump", [IR_BRANCH] = "branch",
    [IR_RETURN] = "ret",
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

static void ir_print_instr(FILE *out, const struct ir_instr *instr) {
  fprintf(out, "  ");
  if (instr->dst != IR_NONE) fprintf(out, "%%%d = ", instr->dst);
  fprintf(out, "%s", ir_op_name(instr->op));

  switch (instr->op) {
    case IR_CONST:  fprintf(out, " %" PRId64, instr->imm.i); break;
    case IR_DCONST: fprintf(out, " %.17g", instr->imm.d); break;
    case IR_STRING: fprintf(out, " <string>"); break;
    case IR_PRINT:
      fprintf(out, ".%s %%%d",
              ir_kind_name(instr->imm.kind), instr->arg1);
      break;

      case IR_JUMP:
        fprintf(out, " L%d", instr->imm.target[0]->id);
        break;

      case IR_BRANCH:
        fprintf(out, " %%%d, L%d, L%d", instr->arg1,
                instr->imm.target[0]->id, instr->imm.target[1]->id);
        break;

    default:
      if (instr->arg1 != IR_NONE) fprintf(out, " %%%d", instr->arg1);
      if (instr->arg2 != IR_NONE) fprintf(out, ", %%%d", instr->arg2);
  }
  fprintf(out, "\n");
}

void ir_print(FILE *out, const struct ir_func *func) {
  for (; func; func = func->next) {
    fprintf(out, "function %s (params=%d, locals=%d, regs=%d)\n",
            func->name, func->nparams, func->nlocals, func->nregs);

    for (const struct ir_block *block = func->head; block; block = block->next) {
      fprintf(out, "L%d:\n", block->id);
      for (const struct ir_instr *instr = block->head; instr; instr = instr->next) {
        ir_print_instr(out, instr);
      }
    }

    fprintf(out, "\n");
  }
}
