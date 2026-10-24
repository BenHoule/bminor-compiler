#!/usr/bin/env bash
#
# Behavioural tests: run each program and compare what it actually does.
#
# For test/programs/good/<name>.bminor:
#   expected/<name>.expected   stdout must match byte-for-byte (required)
#   expected/<name>.status     expected exit status (optional, default 0)
#
# Unlike the per-stage suites, these fixtures are independent of how the
# compiler is built, so the same files check the IR interpreter (Phase 7),
# the native code generator (Phase 8), and every optimization (Phase 9).
#
# Env:
#   BMINOR_BIN=path       compiler to test (default ./bminor)
#   BMINOR_RUN=-interp    stage flag that runs a program (default -interp)
#   BMINOR_ONLY="a b"     run only the named programs
#
# Usage: test/programs/run.sh

set -u

cd "$(dirname "$0")/../.." || exit 1

BIN=${BMINOR_BIN:-./bminor}
RUN=${BMINOR_RUN:--interp}
[[ -x $BIN ]] || { echo "error: $BIN not built; run 'make' first"; exit 1; }

RED=$'\e[31m'; GREEN=$'\e[32m'; DIM=$'\e[2m'; RESET=$'\e[0m'
[[ -t 1 ]] || { RED=; GREEN=; DIM=; RESET=; }

pass=0; fail=0; failed=()
out=$(mktemp); err=$(mktemp)
trap 'rm -f "$out" "$err"' EXIT

printf '\n%s=== programs (%s) ===%s\n' "$DIM" "$RUN" "$RESET"

for file in test/programs/good/*.bminor; do
    name=$(basename "$file" .bminor)
    if [[ -n ${BMINOR_ONLY:-} ]] && [[ " $BMINOR_ONLY " != *" $name "* ]]; then
        continue
    fi

    expected=test/programs/good/expected/$name.expected
    status_file=test/programs/good/expected/$name.status
    want_status=0
    [[ -f $status_file ]] && want_status=$(<"$status_file")

    timeout 10 "$BIN" "$RUN" "$file" >"$out" 2>"$err"
    status=$?

    why=""
    if [[ $status -eq 124 ]]; then
        why="timed out"
    elif [[ $status -ne $want_status ]]; then
        why="exit status $status, expected $want_status"
    elif ! diff -q "$expected" "$out" >/dev/null; then
        why="output mismatch"
    fi

    if [[ -z $why ]]; then
        printf '%s pass%s %s\n' "$GREEN" "$RESET" "$name"
        pass=$((pass+1))
        continue
    fi

    printf '%s FAIL%s %s %s(%s)%s\n' "$RED" "$RESET" "$name" "$DIM" "$why" "$RESET"
    if [[ $why == "output mismatch" ]]; then
        diff -u "$expected" "$out" | tail -n +3 | head -20 | sed 's/^/      /'
    fi
    sed 's/^/      /' "$err" | head -5
    fail=$((fail+1)); failed+=("$name")
done

printf '\n%d passed, %d failed\n' "$pass" "$fail"
if [[ $fail -gt 0 ]]; then
    printf 'failing: %s\n' "${failed[*]}"
    exit 1
fi
exit 0
