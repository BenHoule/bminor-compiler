#include "codegen.h"
#include "ir.h"
#include "param_list.h"
#include "symbol.h"
#include "type.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Naive strategy: every virtual register lives in its own stack slot, and each
// instruction loads its operands into fixed machine registers, computes, and
// stores the result back. Register allocation (Phase 9) removes most of this.

#define SLOT(reg) (-8 * ((reg) + 1))

static FILE *out;
static int func_index;  // keeps block labels unique per function
static int string_count;
static int dims_offset;  // frame offset of current func's temporary (aka scratch) array length var

// Argument registers for x86-64 calling convention (System V AMD64 ABI)
// 1. %rdi – 1st argument (historically destination index / file descriptor in syscalls)
// 2. %rsi – 2nd argument (historically source index)
// 3. %rdx – 3rd argument (historically data register)
// 4. %rcx – 4th argument (historically counter register)
// 5. %r8  – 5th argument (64-bit general-purpose register added in x86-64)
// 6. %r9  – 6th argument (64-bit general-purpose register added in x86-64)
//
// NOTE: If `nargs > 6`: additional arguments are pushed onto the stack.

static const char *const int_args[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
enum { MAX_INT_ARGS = 6, MAX_FP_ARGS = 8 };

[[noreturn]] static void codegen_fail(const char *what) {
  fprintf(stderr, "codegen: %s not supported yet\n", what);
  exit(EXIT_FAILURE);
}

// Load the value of virtual register `reg` into the machine register `x86`
static void codegen_load(int reg, const char *x86) {
  fprintf(out, "\tmovq %d(%%rbp), %s\n", SLOT(reg), x86);
}

// Store the value from the machine register `x86` into the virtual register `reg`
static void codegen_store(const char *x86, int reg) {
  fprintf(out, "\tmovq %s, %d(%%rbp)\n", x86, SLOT(reg));
}

// Load the value of virtual register `reg` into the floating-point register `xmm`
static void codegen_fload(int reg, const char *xmm) {
  fprintf(out, "\tmovsd %d(%%rbp), %s\n", SLOT(reg), xmm);
}

// Store the value from the floating-point register `xmm` into the virtual register `reg`
static void codegen_fstore(const char *xmm, int reg) {
  fprintf(out, "\tmovsd %s, %d(%%rbp)\n", xmm, SLOT(reg));
}

static void codegen_label(const struct ir_block *block) {
  fprintf(out, ".L%d_%d", func_index, block->id);
}


static void codegen_string(const char *str) {
  // "" prefixes multiline string args because I have parameter name hints turned on which fucks up
  // alignment for the first line (trying to keep the assembly vaguely readable)
  fprintf(out, ""
    "\t.section .rodata\n"
    ".LS%d:\n"
    "\t.string \"",
    string_count
  );

  // Octal escapes keep the assembler from interpreting quotes, backslashes, or
  // control characters in the string.
  for (const unsigned char *c = (const unsigned char *)str; *c; c++) {
    const bool needs_escape = *c == '"' || *c == '\\' || *c < 32 || *c > 126;
    if (needs_escape) fprintf(out, "\\%03o", *c);
    else fputc(*c, out);
  }

  fprintf(out, "\"\n"
    "\t.text\n"
    "\tleaq .LS%d(%%rip), %%rax\n",
    string_count++
  );
}

static type_t codegen_return_kind(const struct ir_func *func) {
  return func->symbol ? func->symbol->type->subtype->kind : TYPE_VOID;
}

static void codegen_call(const struct ir_instr *instr) {
  const struct type *ftype = instr->imm.sym->type;
  const struct param_list *param = ftype->params;
  int nint = 0, nfp = 0;

  for (int i = 0; i < instr->nargs; i++, param = param->next) {
    if (param->type->kind == TYPE_DOUBLE) {
      if (nfp >= MAX_FP_ARGS) {
        // TODO: change to ("more than %d...", MAX_FP_ARGS); (?) idk how likely the value is to change
        codegen_fail("more than 8 double arguments");
        unreachable();
      }
      fprintf(out, "\tmovsd %d(%%rbp), %%xmm%d\n", SLOT(instr->args[i]), nfp++);
    } else if (nint >= MAX_INT_ARGS) {
      // TODO: change to ("more than %d...", MAX_INT_ARGS); (?) idk how likely the value is to change
      codegen_fail("more than 6 integer arguments");
      unreachable();
    } else codegen_load(instr->args[i], int_args[nint++]);
  }

  fprintf(out, "\tmovl $%d, %%eax\n", nfp);  // vector-register count for variadic callees
  fprintf(out, "\tcall %s\n", instr->imm.sym->name);

  if (instr->dst == IR_NONE) return;
  if (ftype->subtype->kind == TYPE_DOUBLE) codegen_fstore("%xmm0", instr->dst);
  else codegen_store("%rax", instr->dst);
}

static void codegen_print(const struct ir_instr *instr) {
  const char *callee = nullptr;
  switch (instr->imm.kind) {
    case TYPE_INTEGER: callee = "print_integer"; break;
    case TYPE_BOOLEAN: callee = "print_boolean"; break;
    case TYPE_CHAR:    callee = "print_character"; break;
    case TYPE_STRING:  callee = "print_string"; break;
    case TYPE_DOUBLE:
      codegen_fload(instr->arg1, "%xmm0");
      fprintf(out, ""
        "\tmovl $1, %%eax\n"
        "\tcall print_double\n"
      );
      return;

    default: unreachable();
  }
  codegen_load(instr->arg1, "%rdi");
  fprintf(out, "\tcall %s\n", callee);
}

static void codegen_int_binary(const struct ir_instr *instr, const char *op) {
  codegen_load(instr->arg1, "%rax");
  codegen_load(instr->arg2, "%rcx");
  fprintf(out, "\t%s %%rcx, %%rax\n", op);
  codegen_store("%rax", instr->dst);
}

static void codegen_fp_binary(const struct ir_instr *instr, const char *op) {
  codegen_fload(instr->arg1, "%xmm0");
  codegen_fload(instr->arg2, "%xmm1");
  fprintf(out, "\t%s %%xmm1, %%xmm0\n", op);
  codegen_fstore("%xmm0", instr->dst);
}

static void codegen_divide(const struct ir_instr *instr) {
  codegen_load(instr->arg1, "%rax");
  codegen_load(instr->arg2, "%rcx");
  fprintf(out, ""
      "\ttestq %%rcx, %%rcx\n"
      "\tjnz 1f\n"
      "\tcall bminor_division_by_zero\n"
    "1:\n"
      "\tcqto\n"
      "\tidivq %%rcx\n"
  );
  codegen_store(instr->op == IR_DIV ? "%rax" : "%rdx", instr->dst);
}

static void codegen_int_compare(const struct ir_instr *instr) {
  static const char *const setcc[] = {
    [IR_EQ] = "sete", [IR_NE] = "setne",
    [IR_LT] = "setl", [IR_LE] = "setle",
    [IR_GT] = "setg", [IR_GE] = "setge"
  };
  codegen_load(instr->arg1, "%rax");
  codegen_load(instr->arg2, "%rcx");
  fprintf(out, ""
      "\tcmpq %%rcx, %%rax\n"
      "\t%s %%al\n"
      "\tmovzbq %%al, %%rax \n",
      setcc[instr->op]
  );
  codegen_store("%rax", instr->dst);
}

// ucomisd sets flags like an unsigned compare and flags NaN as unordered (PF).
// Testing "above"/"above or equal" with swapped operands makes NaN compare false.
static void codegen_fp_compare(const struct ir_instr *instr) {
  ir_op_t op = instr->op;
  bool swap = op == IR_FLT || op == IR_FLE;
  codegen_fload(swap ? instr->arg2 : instr->arg1, "%xmm0");
  codegen_fload(swap ? instr->arg1 : instr->arg2, "%xmm1");
  fprintf(out, "\tucomisd %%xmm1, %%xmm0\n");

  switch (op) {
    case IR_FEQ:
      fprintf(out, ""
        "\tsete %%al\n"
        "\tsetnp %%cl\n"
        "\tandb %%cl, %%al\n"
      ); break;

    case IR_FNE:
      fprintf(out, ""
        "\tsetne %%al\n"
        "\tsetp %%cl\n"
        "\torb %%cl, %%al\n"
      ); break;

    case IR_FLT: case IR_FGT: fprintf(out, "\tseta %%al\n"); break;
    case IR_FLE: case IR_FGE: fprintf(out, "\tsetae %%al\n"); break;

    default: unreachable();
  }

  fprintf(out, "\tmovzbq %%al, %%rax\n");
  codegen_store("%rax", instr->dst);
}

// Lengths are copied into a contiguous scratch area below the register slots,
// because bminor_array_new takes them as an int64_t array.
static void codegen_array(const struct ir_instr *instr) {
  for (int i = 0; i < instr->nargs; i++) {
    codegen_load(instr->args[i], "%rax");
    fprintf(out, "\tmovq %%rax, %d(%%rbp)\n", dims_offset + i * 8);
  }
  fprintf(out, ""
    "\tleaq %d(%%rbp), %%rdi\n"
    "\tmovq $%d, %%rsi\n"
    "\tcall bminor_array_new\n",
    dims_offset, instr->nargs
  );
  codegen_store("%rax", instr->dst);
}

static void codegen_index(const struct ir_instr *instr) {
  codegen_load(instr->arg1, "%rax");
  codegen_load(instr->arg2, "%rcx");
  if (instr->imm.i)
    fprintf(out, ""
      "\tcmpq (%%rax), %%rcx\n"
      "\tjb 1f\n"  // if (0 <= index < length) goto 1:
      "\tcall bminor_bounds_error\n"
      "\t1:\n"
    );
  fprintf(out, "\tleaq 8(%%rax,%%rcx,8), %%rax\n");
  codegen_store("%rax", instr->dst);
}

static void codegen_instr(const struct ir_func *func, const struct ir_instr *instr) {
  switch (instr->op) {
    case IR_CONST:
      fprintf(out, "\tmovabsq $%" PRId64 ", %%rax\n", instr->imm.i);
      codegen_store("%rax", instr->dst);
      break;

    case IR_DCONST: {
      uint64_t bits;
      memcpy(&bits, &instr->imm.d, sizeof bits);
      fprintf(out, "\tmovabsq $%" PRIu64 ", %%rax\n", bits);
      codegen_store("%rax", instr->dst);
      break;
    }

    case IR_STRING:
      codegen_string(instr->imm.s);
      codegen_store("%rax", instr->dst);
      break;

    case IR_COPY:
      codegen_load(instr->arg1, "%rax");
      codegen_store("%rax", instr->dst);
      break;

    case IR_ADD: codegen_int_binary(instr, "addq"); break;
    case IR_SUB: codegen_int_binary(instr, "subq"); break;
    case IR_MUL: codegen_int_binary(instr, "imulq"); break;
    case IR_DIV:
    case IR_MOD: codegen_divide(instr); break;
    case IR_POW:
      codegen_load(instr->arg1, "%rdi");
      codegen_load(instr->arg2, "%rsi");
      fprintf(out, "\tcall integer_power\n");
      codegen_store("%rax", instr->dst);
      break;

    case IR_NEG:
    case IR_FNEG:
      codegen_load(instr->arg1, "%rax");
      fprintf(out, instr->op == IR_NEG
        ? "\tnegq %%rax\n"
        : "\tbtcq $63, %%rax\n"
      );
      codegen_store("%rax", instr->dst);
      break;

    case IR_FADD: codegen_fp_binary(instr, "addsd"); break;
    case IR_FSUB: codegen_fp_binary(instr, "subsd"); break;
    case IR_FMUL: codegen_fp_binary(instr, "mulsd"); break;
    case IR_FDIV: codegen_fp_binary(instr, "divsd"); break;

    case IR_EQ: case IR_NE: case IR_LT: case IR_LE: case IR_GT: case IR_GE:
      codegen_int_compare(instr);
      break;

    case IR_FEQ: case IR_FNE: case IR_FLT: case IR_FLE: case IR_FGT: case IR_FGE:
      codegen_fp_compare(instr);
      break;

    case IR_NOT:
      codegen_load(instr->arg1, "%rax");
      fprintf(out, "\txorq $1, %%rax \n");
      codegen_store("%rax", instr->dst);
      break;

    case IR_PRINT: codegen_print(instr); break;

    case IR_GLOBAL:
      fprintf(out, "\tleaq .LG_%s(%%rip), %%rax\n", instr->imm.sym->name);
      codegen_store("%rax", instr->dst);
      break;

    case IR_LOAD:
      codegen_load(instr->arg1, "%rax");
      fprintf(out, "\tmovq (%%rax), %%rax\n");
      codegen_store("%rax", instr->dst);
      break;

    case IR_STORE:
      codegen_load(instr->arg1, "%rax");
      codegen_load(instr->arg2, "%rcx");
      fprintf(out, "\tmovq %%rcx, (%%rax)\n");
      break;

    case IR_ARRAY: codegen_array(instr); break;
    case IR_INDEX: codegen_index(instr); break;
    case IR_LEN:
      codegen_load(instr->arg1, "%rax");
      fprintf(out, "\tmovq (%%rax), %%rax\n");
      codegen_store("%rax", instr->dst);
      break;

    case IR_CALL: codegen_call(instr); break;

    case IR_JUMP:
      fprintf(out, "\tjmp ");
      codegen_label(instr->imm.target[0]);
      fprintf(out, "\n");
      break;

    case IR_BRANCH:
      fprintf(out, ""
        "\tcmpq $0, %d(%%rbp)\n"
        "\tjne ",
        SLOT(instr->arg1)
      );
      codegen_label(instr->imm.target[0]);
      fprintf(out, "\n\tjmp ");
      codegen_label(instr->imm.target[1]);
      fprintf(out, "\n");
      break;

    case IR_RETURN:
      if (instr->arg1 == IR_NONE) fprintf(out, "\txorl %%eax, %%eax\n");
      else if (codegen_return_kind(func) == TYPE_DOUBLE) codegen_fload(instr->arg1, "%xmm0");
      else codegen_load(instr->arg1, "%rax");
      fprintf(out, "\tleave\n\tret\n");
      break;
  }
}

// Spill each incoming argument register into its parameter's stack slot.
static void codegen_params(const struct ir_func *func) {
  if (!func->symbol) return;

  int nint = 0, nfp =0, i = 0;
  for (const struct param_list *p = func->symbol->type->params; p; p = p->next, i++) {
    if (p->type->kind == TYPE_DOUBLE) {
      if (nfp >= MAX_FP_ARGS) {
        // TODO: change to ("more than %d...", MAX_FP_ARGS); (?) idk how likely the value is to change
        codegen_fail("more than 8 double arguments");
        unreachable();
      }
      fprintf(out, "\tmovsd %%xmm%d, %d(%%rbp)\n", nfp++, SLOT(i));
    } else if (nint >= MAX_INT_ARGS) {
        // TODO: change to ("more than %d...", MAX_INT_ARGS); (?) idk how likely the value is to change
        codegen_fail("more than 6 integer arguments");
        unreachable();
    } else codegen_store(int_args[nint++], i);
  }
}

static void codegen_func(const struct ir_func *func, const char *label, bool exported) {
  bool is_main = strcmp(label, "main") == 0;
  if (is_main && func->nparams > 0) {
    codegen_fail("main function cannot have parameters");
    unreachable();
  }

  int max_dims = 0;
  for (const struct ir_block *block = func->head; block; block = block->next)
    for (const struct ir_instr *instr = block->head; instr; instr = instr->next)
      if (instr->op == IR_ARRAY && instr->nargs > max_dims)
        max_dims = instr->nargs;

  int slots = func->nregs + max_dims;
  dims_offset = slots * -8;
  int frame = (slots * 8 + 15) & ~15;  // keeps %rsp 16-byte aligned at calls
  if (exported) fprintf(out, "\n\t.globl %s\n", label);
  fprintf(out, ""
    "\t.type %s, @function\n"
    "%s:\n"
    "\tpushq %%rbp\n"
    "\tmovq %%rsp, %%rbp\n",
    label, label
  );
  if(frame) fprintf(out, "\tsubq $%d, %%rsp\n", frame);

  codegen_params(func);
  if (is_main) fprintf(out, "\tcall __bminor_init\n");

  for (const struct ir_block *block = func->head; block; block = block->next) {
    codegen_label(block);
    fprintf(out, ":\n");
    for (const struct ir_instr *instr = block->head; instr; instr = instr->next) {
      codegen_instr(func, instr);
    }
  }
  func_index++;
}

static void codegen_find_globals(const struct ir_func *func, const struct symbol **found) {
  for (const struct ir_block *block = func->head; block; block = block->next)
    for (const struct ir_instr *instr = block->head; instr; instr = instr->next)
      if (instr->op == IR_GLOBAL) found[instr->imm.sym->which] = instr->imm.sym;
}

static void codegen_globals(const struct ir_program *prog) {
  if (prog->nglobals == 0) return;

  const struct symbol **found = calloc((size_t)prog->nglobals, sizeof *found);
  if (!found) {
    fprintf(stderr, "codegen: out of memory\n");
    exit(EXIT_FAILURE);
  }

  codegen_find_globals(prog->init, found);
  for (const struct ir_func *func = prog->funcs; func; func = func->next)
    codegen_find_globals(func, found);

  fprintf(out, "\n"
    "\t.bss\n"
    "\t.p2align 3\n"
  );
  for (int i = 0; i < prog->nglobals; i++) {
    if (found[i]) {
      fprintf(out, ""
        ".LG_%s:\n"
        "\t.zero 8\n",
        found[i]->name
      );
    }
  }
  free(found);
}

void codegen_program(FILE *stream, const struct ir_program *program) {
  out = stream;
  func_index = 0;
  string_count = 0;

  fprintf(out, "\t.text\n");
  codegen_func(program->init, "__bminor_init", false);
  for (const struct ir_func *func = program->funcs; func; func = func->next)
    codegen_func(func, func->name, true);

  codegen_globals(program);
  fprintf(out, "\t.section .note.GNU-stack, \"\",@progbits\n");
}
