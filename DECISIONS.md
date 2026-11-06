- `ES  (\\(0x{H}{H}|[^\n]))` vs `ES  (\\(0x{H}{H}|[^abefnrtv]))`
  - First is simpler and forces strings to be terminated on the same line.
  - Second would allow formatting multiline strings by escaping the newline. Technically closer to the given specification that "*any* character after a '\' is escaped".

- `-9223372036854775808` is parsed as unary minus applied to `9223372036854775808`, which is 1 larger than the max possible value of a `long long` (thank you 2s complement). Most compilers treat this as a known limitation: Reject it, or accept a value up to 2⁶³ in the scanner and have the parser check it next to unary minus. Also, decide whether `0xFFFFFFFFFFFFFFFF` is valid (a bit pattern) or an overflow; if it's valid, parse hex with strtoull.

- **AST Invariant:** After successful resolution and typechecking, every value expression has a concrete type, every name has a symbol, and every function signature has a concrete return type.

- Uninitialized locals start at 0 (matches the spec's rule for arrays and keeping runs deterministic).

- No mod/pow operations on doubles (for now)

- `ir_program` holds a synthetic `$init` function, which stores each global's initial value and runs before main.

- `EXPR_ASGN` lowers target before the assigned value, so `a[i] = i++` indexes with `i` before incrementing it (C leaves the order unspecified).

- **Every virtual register gets a stack slot at `-8*(r+1)(%rbp)`.** Each instruction loads operands into `%rax`/`%rcx` (or `%xmm0`/`%xmm1`), computes, and stores the result back. Slow but convenient for now (will be replaced by register allocator).

- **Stack alignment:** after `push %rbp` the stack is 16-byte aligned; rounding the frame up to 16 keeps it aligned at every call. Misalignment usually only crashes inside libc's `printf` (aligned SSE stores).

- **`%al` before calls:** variadic functions read `%al` to know how many `xmm` registers hold arguments. Setting it for every call covers `printf` if it is ever called directly.

- **Integer vs double arguments:** Draw from separate register sequences. In `f(integer a, double b, integer c)`, `a` goes in `rdi`, `b` in `xmm0` and `c` in `rsi`. Requires accessing parameter types with `ir_func->symbol`.

- **PIE:** gcc links position-independent executables by default; absolute addresses like `movq $label` don't link. Strings/globals are reached with `leaq sym(%rip)`.

- **Globals:** Live in `.bss`, zero-initialized, and main calls `__bminor_init` first (mirrors the interpreter). `.LG_` prefix stops a global named `print_integer` from clashing with the runtime.

- **NaN-correct float compares:** `ucomisd` reports "unordered" through `PF`, `ZF` and `CF` all set. Testing `a < b` as `b > a` with `seta (CF=0 and ZF=0)` makes `NaN` compare `false` for free. `==` needs `ZF=1` and `PF=0`, and `!=` needs `ZF=0` or `PF=1`.

- **`-x` for doubles** flips the sign bit with `btcq $63` instead of computing `0 - x`. Subtraction would give `+0.0` for `-(0.0)`.
