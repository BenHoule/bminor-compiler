#!/usr/bin/env bash
#
# Checks that name resolution assigns correct storage positions.
#
# The good/bad suite only checks exit status, so it cannot catch a resolver that
# resolves every name successfully but gives every local the same `which` value.
# That bug is invisible until code generation emits overlapping stack slots.
#
# Requires `bminor -resolve` to print one line per binding in this format:
#     <name> <global|local|param> <which>
# e.g.
#     x global -1
#     p local 0
#     q local 1
#
# Usage: test/resolver/positions.sh

set -u

cd "$(dirname "$0")/../.." || exit 1

BIN=${BMINOR:-./bminor}
[[ -x $BIN ]] || { echo "error: $BIN not built; run 'make' first"; exit 1; }

RED=$'\e[31m'; GREEN=$'\e[32m'; DIM=$'\e[2m'; RESET=$'\e[0m'
[[ -t 1 ]] || { RED=; GREEN=; DIM=; RESET=; }

pass=0; fail=0
src=$(mktemp --suffix=.bminor); got=$(mktemp)
trap 'rm -f "$src" "$got"' EXIT

# check <description> <program> <expected grep -E pattern on the full output>
check() {
    local desc=$1 prog=$2 pattern=$3
    printf '%s\n' "$prog" >"$src"

    if ! "$BIN" -resolve "$src" >"$got" 2>&1; then
        printf '%s FAIL%s %s %s(resolve failed)%s\n' "$RED" "$RESET" "$desc" "$DIM" "$RESET"
        sed 's/^/      /' "$got"
        fail=$((fail+1)); return
    fi

    if grep -qE "$pattern" "$got"; then
        printf '%s pass%s %s\n' "$GREEN" "$RESET" "$desc"
        pass=$((pass+1))
    else
        printf '%s FAIL%s %s %s(expected /%s/)%s\n' "$RED" "$RESET" "$desc" "$DIM" "$pattern" "$RESET"
        sed 's/^/      /' "$got"
        fail=$((fail+1))
    fi
}

printf '\n%s=== resolver storage positions ===%s\n' "$DIM" "$RESET"

check "globals are SYMBOL_GLOBAL" \
      'g: integer = 1;' \
      '^g[[:space:]]+global'

check "function names are SYMBOL_GLOBAL" \
      'main: function integer () = { return 0; }' \
      '^main[[:space:]]+global'

check "locals number from 0 upward" \
      'main: function integer () = { p: integer; q: integer; r: integer; return 0; }' \
      '^p[[:space:]]+local[[:space:]]+0'

check "second local is slot 1" \
      'main: function integer () = { p: integer; q: integer; r: integer; return 0; }' \
      '^q[[:space:]]+local[[:space:]]+1'

check "third local is slot 2" \
      'main: function integer () = { p: integer; q: integer; r: integer; return 0; }' \
      '^r[[:space:]]+local[[:space:]]+2'

check "params number from 0" \
      'f: function integer (a: integer, b: integer) = { return a; }' \
      '^a[[:space:]]+param[[:space:]]+0'

check "second param is index 1" \
      'f: function integer (a: integer, b: integer) = { return a; }' \
      '^b[[:space:]]+param[[:space:]]+1'

check "local counter restarts per function" \
      'f: function integer () = { u: integer; return 0; } g: function integer () = { v: integer; return 0; }' \
      '^v[[:space:]]+local[[:space:]]+0'

printf '\n%d passed, %d failed\n' "$pass" "$fail"
[[ $fail -eq 0 ]]
