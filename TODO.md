**1. Make expression printing precedence-aware**
- Pass precedence level + associativity (optional) to recursive printing calls.
- Wrap child in parentheses only if `child_precedence <= parent_precedence`.

**2. Expand `roundtrip.sh` beyond byte-idempotence and binding-trace comparison.**
- Cannot detect consistently printed but semantically different expression.
- Add structural AST comparison/targeted precedence tests, plus successful re-typechecking of valid typed fixtures.

**3. Add location (line/col) info to typecheck errors(?)**
- Typechecking occurs via AST, does the information even exist at this stage?
- If not, is it worth attaching it to AST nodes?

**4. Update AST traversal to be thread safe**
- Morris algorithm is not thread safe (edits AST directly).
- Trying to minimize recursion to save the stack some grief.

**5. Fix Array Size Typechecking**
- `type_typecheck` rejects all negative expressions, even if it shouldn't; e.g. `[-(-1)]`
- Need to evaluate constant expressions to handle negative literals correctly

**6. Testing (General)**
- `string_decode`: exact-fit, one-byte-short, empty-string, and maximally escaped tests

