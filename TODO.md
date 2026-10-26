**1. Testing (General)**
- `string_decode`: exact-fit, one-byte-short, empty-string, and maximally escaped tests

**2. Make expression printing precedence-aware**
- Pass precedence level + associativity (optional) to recursive printing calls.
- Wrap child in parentheses only if `child_precedence <= parent_precedence`.

**3. Expand `roundtrip.sh` beyond byte-idempotence and binding-trace comparison.**
- Cannot detect consistently printed but semantically different expression.
- Add structural AST comparison/targeted precedence tests, plus successful re-typechecking of valid typed fixtures.

**4. Update AST traversal to be thread safe**
- Morris algorithm is not thread safe (edits AST directly).
- Trying to minimize recursion to save the stack some grief.

**5. Add location (line/col) info to name-resolution/typecheck errors(?)**
- Both stages traverse AST, does the information even exist at this point?
- If not, is it worth attaching it to AST nodes?

**6. Fix Array Size Typechecking**
- `type_typecheck` rejects all negative expressions, even if it shouldn't; e.g. `[-(-1)]`
- Need to evaluate constant expressions to handle negative literals correctly
- Add tests for nested negations in array size
