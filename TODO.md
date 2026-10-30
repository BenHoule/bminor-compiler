**1. Testing (General)**
- Done: `string_decode` exact-fit, one-byte-short, empty-string, and maximally escaped tests.
- Done: scanner reopening, runtime ABI/power, and deep resolver tests.

**2. Make expression printing precedence-aware**
- Optional formatting improvement; current fully parenthesized printer preserves meaning.
- A compact printer must consider operand position and associativity as well as precedence.

**3. Expand `roundtrip.sh` beyond byte-idempotence and binding-trace comparison.**
- Done: targeted exact-output precedence/literal fixtures and re-typechecking of well-typed inputs.
- Optional: structural AST comparison for broader property testing.

**4. Non-mutating AST traversal**
- Done: iterative resolver traversal uses an explicit stack and preserves child links.
- This does not make the compiler reentrant; scanner/resolver state is still global.

**5. Add location (line/col) info to name-resolution/typecheck errors(?)**
- Both stages traverse AST, does the information even exist at this point?
- If not, is it worth attaching it to AST nodes?

**6. Array Size Typechecking**
- Done: shared constant evaluation handles nested negation/arithmetic and initializer counts.
- Runtime-dependent integer dimensions remain allowed and require runtime checks during lowering.

**7. Typed AST invariant**
- Done: read-only validation checks concrete types and bound names before lowering.
- List/initializer containers are explicitly exempt from value-type requirements.
