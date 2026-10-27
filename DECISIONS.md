- `ES  (\\(0x{H}{H}|[^\n]))` vs `ES  (\\(0x{H}{H}|[^abefnrtv]))`
  - First is simpler and forces strings to be terminated on the same line.
  - Second would allow formatting multiline strings by escaping the newline. Technically closer to the given specification that "*any* character after a '\' is escaped".

- `-9223372036854775808` is parsed as unary minus applied to `9223372036854775808`, which is 1 larger than the max possible value of a `long long` (thank you 2s complement). Most compilers treat this as a known limitation: Reject it, or accept a value up to 2⁶³ in the scanner and have the parser check it next to unary minus. Also, decide whether `0xFFFFFFFFFFFFFFFF` is valid (a bit pattern) or an overflow; if it's valid, parse hex with strtoull.

- **AST Invariant:** After successful resolution and typechecking, every value expression has a concrete type, every name has a symbol, and every function signature has a concrete return type.
