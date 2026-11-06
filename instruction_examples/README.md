# C -> LLVM IR -> x86-64

Small, independent functions for comparing C operations with LLVM instructions
and their x86-64 lowering.

Requires Clang with the x86 backend and GNU Make. From the project root:

```sh
make -C instruction_examples -j
```

Each `name.c` produces `llvm/name.ll`, then `x86/name.s` **from that same IR**.
Use `make -C instruction_examples llvm` or `make -C instruction_examples x86`
to select an output stage. `make -C instruction_examples clean` removes the
generated files. Override the compiler with `CLANG=clang-18`, for example.
Adding another `.c` file automatically includes it in the next build.

## Preserving structure

- Both stages use `-O0`
- `-fno-inline -fno-builtin -fno-stack-protector` avoids inlining, builtin
  substitutions, and stack-protector instrumentation in the C frontend.
- `-fno-omit-frame-pointer` keeps recognizable stack frames.
- `-fno-discard-value-names` keeps readable LLVM names.
- `-gline-tables-only` adds source locations (IR `!dbg` and assembly `.loc`)
  without full variable/type debug information.
- The target is explicitly `x86_64-unknown-linux-gnu` (System V AMD64 ABI). Assembly uses AT&T syntax: source before destination, `%` registers, and `$` immediates.
- An LLVM `alloca` often becomes a stack offset rather than a distinct instruction.
- `getelementptr` can become an address operand or `lea`.
- Unoptimized code also includes stack traffic that is not intrinsic to the operation. Exact output varies by Clang version.
- No `volatile` is added (changes memory semantics).

## What to compare

| Source | LLVM operations | Typical x86 instructions/features |
|--------|-----------------|----------------------------------|
| `arithmetic.c` | `add`, `sub`, `mul`, `sdiv`, `udiv`, `srem`, `urem` | `add`, `sub`, `imul`, `idiv`, `div`; quotient/remainder registers |
| `bitwise.c` | `and`, `or`, `xor`, `shl`, `lshr`, `ashr` | `and`, `or`, `xor`, `shl`, `shr`, `sar`; variable counts in `%cl` |
| `comparisons.c` | signed/unsigned `icmp`, `zext` | `cmp`, signed/unsigned `setcc`, `movzbl` |
| `control_flow.c` | `br`, `phi`, `icmp` | conditional jumps, boolean materialization, merges |
| `loops.c` | backedges, loads/stores, `br` | loop labels, comparisons, jumps |
| `memory.c` | `alloca`, `load`, `store`, `getelementptr`, globals | `mov`, scaled addressing, structure offsets, RIP-relative addressing |
| `calls.c` | `call`, `ret` | `call`, `ret`, six integer argument registers, stack arguments |
| `floating_point.c` | `fadd`, `fsub`, `fmul`, `fdiv`, `fcmp` | scalar SSE2 arithmetic, floating comparisons, XMM registers |
| `conversions.c` | `sext`, `zext`, `trunc`, `sitofp`, `fptosi`, `fpext`, `fptrunc` | sign/zero extension, subregisters, integer/float conversions |

For example, compare `signed_divide` and `unsigned_divide` in
`arithmetic.c`, `llvm/arithmetic.ll`, and `x86/arithmetic.s`. Then compare
the remainder functions: x86 division computes both quotient and remainder.

Inputs must obey C rules: valid pointers and indices, shift counts below 32,
no signed overflow, nonzero divisors, and no `INT_MIN / -1` (or remainder).
Floating-to-integer conversions require finite, representable values. Negative
signed right shifts are implementation-defined in C17; this Clang target uses
arithmetic right shift. Floating comparisons retain normal NaN semantics.
