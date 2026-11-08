#include "optimizer.h"
#include "ir.h"
#include "memory.h"

#include <stdint.h>
#include <string.h>

static void opt_set_const(struct ir_instr *instr, int64_t value) {
  instr->op = IR_CONST;
  instr->arg1 = instr->arg2 = IR_NONE;
  instr->imm.i = value;
}

static void opt_set_copy(struct ir_instr *instr, int src) {
  instr->op = IR_COPY;
  instr->arg1 = src;
  instr->arg2 = IR_NONE;
}

// Arithmetic goes through uint64_t so overflow wraps like x86-64 instead of being UB.
static bool opt_fold_binop(ir_op_t op, int64_t a, int64_t b, int64_t *result) {
  switch (op) {
    case IR_ADD: *result = (int64_t)((uint64_t)a + (uint64_t)b); return true;
    case IR_SUB: *result = (int64_t)((uint64_t)a - (uint64_t)b); return true;
    case IR_MUL: *result = (int64_t)((uint64_t)a * (uint64_t)b); return true;
    case IR_DIV:
    case IR_MOD:
      // Division by zero must still fail at runtime; INT64_MIN / -1 traps in idivq.
      if (b == 0 || (a == INT64_MIN && b == -1)) return false;
      *result = op == IR_DIV ? a / b : a % b;
      return true;

    case IR_EQ: *result = a == b; return true;
    case IR_NE: *result = a != b; return true;
    case IR_LT: *result = a <  b; return true;
    case IR_LE: *result = a <= b; return true;
    case IR_GT: *result = a >  b; return true;
    case IR_GE: *result = a >= b; return true;

    default: return false;
  }
}

static void opt_simplify(struct ir_instr *instr, bool k1, int64_t a, bool k2, int64_t b) {
  switch (instr->op) {
    case IR_ADD:
      if (k1 && a == 0) opt_set_copy(instr, instr->arg2);
      else if (k2 && b == 0) opt_set_copy(instr, instr->arg1);
      break;

    case IR_SUB:
      if (k2 && b == 0) opt_set_copy(instr, instr->arg1);
      break;

    case IR_MUL:
      if ((k1 && a == 0) || (k2 && b == 0)) opt_set_const(instr, 0);
      else if (k1 && a == 1) opt_set_copy(instr, instr->arg2);
      else if (k2 && b == 1) opt_set_copy(instr, instr->arg1);
      break;

    case IR_DIV:
      if (k2 && b == 1) opt_set_copy(instr, instr->arg1);
      break;

    default: break;
  }
}

static void opt_fold_instr(struct ir_instr *instr, bool *known, int64_t *value) {
  bool k1 = instr->arg1 != IR_NONE && known[instr->arg1];
  bool k2 = instr->arg2 != IR_NONE && known[instr->arg2];
  int64_t a = k1 ? value[instr->arg1] : 0;
  int64_t b = k2 ? value[instr->arg2] : 0;

  switch (instr->op) {
    case IR_COPY:
      if (k1) opt_set_const(instr, a);
      return;

    case IR_NEG:
      if (k1) opt_set_const(instr, (int64_t)(0 - (uint64_t) a));
      return;

    case IR_NOT:
      if (k1) opt_set_const(instr, !a);
      return;

    case IR_BRANCH:
      if (k1) {
        instr->op = IR_JUMP;
        instr->imm.target[0] = instr->imm.target[a ? 0 : 1];
        instr->arg1 = IR_NONE;
      }
      return;

    default: break;
  }

  int64_t result;
  if (k1 && k2 && opt_fold_binop(instr->op, a, b, &result)) opt_set_const(instr, result);
  else opt_simplify(instr, k1, a, k2, b);
}

// Constants are only tracked within a block: a register written in one block
// may be rewritten on another path before this one runs.
static void opt_fold_constants(struct ir_func *func) {
  size_t n = (size_t)func->nregs + 1;
  bool *known = memory_allocate(n * sizeof *known);
  int64_t *value = memory_allocate(n * sizeof *value);

  for (struct ir_block *block = func->head; block; block = block->next) {
    memset(known, 0, n * sizeof *known);
    for (struct ir_instr *instr = block->head; instr; instr = instr->next) {
      opt_fold_instr(instr, known, value);
      if (instr->dst == IR_NONE) continue;
      known[instr->dst] = instr->op == IR_CONST;
      value[instr->dst] = instr->imm.i;
    }
  }
}

static void opt_remove_unreachable(struct ir_func *func) {
  size_t n = (size_t)func->nblocks+ 1;
  bool *reached = memory_allocate(n * sizeof *reached);
  struct ir_block **stack = memory_allocate(n * sizeof *stack);

  int top = 0;
  stack[top++] = func->head;
  reached[func->head->id] = true;
  while (top > 0) {
    struct ir_block *succ[2];
    int nsucc = ir_block_successors(stack[--top], succ);
    for (int i = 0; i < nsucc; i++) {
      if (reached[succ[i]->id]) continue;
      reached[succ[i]->id] = true;
      stack[top++] = succ[i];
    }
  }

  struct ir_block *kept = nullptr;
  for (struct ir_block *block = func->head, *next; block; block = next) {
    next = block->next;
    if (!reached[block->id]) continue;
    if (kept) kept->next = block;
    else func->head = block;
    kept = block;
  }
  if (kept) kept->next = nullptr;
  func->tail = kept;
}

static void optimize_function(struct ir_func *func) {
  if (func == nullptr) return;
  opt_fold_constants(func);
  opt_remove_unreachable(func);
}

void optimize_program(struct ir_program *prog) {
  if (prog == nullptr) return;
  optimize_function(prog->init);
  for (struct ir_func *func = prog->funcs; func; func = func->next)
    optimize_function(func);
}
