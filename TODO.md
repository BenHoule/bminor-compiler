**1. Testing (General - never truly done)**
- [DONE]: `string_decode` exact-fit, one-byte-short, empty-string, and maximally escaped tests.
- [DONE]: scanner reopening, runtime ABI/power, and deep resolver tests.

**2. Make expression printing precedence-aware**
- Optional formatting improvement; current fully parenthesized printer preserves meaning.
- A compact printer must consider operand position and associativity as well as precedence.

**3. [DONE] Expand `roundtrip.sh` beyond byte-idempotence and binding-trace comparison.**
- Exact-output precedence/literal fixtures and re-typechecking of well-typed inputs.
- Optional: structural AST comparison for broader property testing.

**4. [DONE] Non-mutating AST traversal**
- Iterative resolver traversal uses an explicit stack and preserves child links.
- This does not make the compiler reentrant; scanner/resolver state is still global.

**5. Add location (line/col) info to name-resolution/typecheck errors(?)**
- Both stages traverse AST, does the information even exist at this point?
- If not, is it worth attaching it to AST nodes?

**6. [DONE] Array Size Typechecking**
- shared constant evaluation handles nested negation/arithmetic and initializer counts.
- Runtime-dependent integer dimensions remain allowed and require runtime checks during lowering.

**7. [DONE] Typed AST invariant**
- Read-only validation checks concrete types and bound names before lowering.
- List/initializer containers are explicitly exempt from value-type requirements.

**8. interpreter adding bug**
- `arg1.i + arg2.i` is undefined behaviour in C on signed overflow; native code wraps.
- Compute through `uint64_t` so they stay identical once Phase 9 differential testing starts.

**9. Fix name clashes with library.c functions**
- User function names are emitted as written
- B-Minor function named exit or print_string would clash with libc or the runtime.

**10. Deferred Features**
- `main(argc, argv)` (no command-line input to main).
- `carray`  interop with C (No passing/receiving raw, unmanaged C arrays across the language boundary).
