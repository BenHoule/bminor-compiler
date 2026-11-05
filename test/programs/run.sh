#!/usr/bin/env bash
#
# Behavioural tests: run each program and compare what it actually does.
#
# For test/programs/good/<name>.bminor:
#   expected/<name>.expected   stdout must match byte-for-byte (required)
#   expected/<name>.status     expected exit status (optional, default 0)
#
# Unlike the per-stage suites, these fixtures are independent of how the
# compiler is built, so the same files check the IR interpreter,
# the native code generator, and every optimization.
#
# Env:
#   BMINOR_BIN=path       compiler to test (default $BMINOR, then ./bminor)
#   BMINOR_RUN=-interp    -interp runs the IR interpreter; -codegen compiles,
#                         links with BMINOR_RUNTIME, and runs the executable
#   BMINOR_RUNTIME=path   runtime object for -codegen (default build/library.o)
#   BMINOR_CC=cc          assembler/linker driver (default gcc-14)
#   BMINOR_ONLY="a b"     run only the named programs
#
# Usage: test/programs/run.sh

set -u

cd "$(dirname "$0")/../.." || exit 1

BIN=${BMINOR_BIN:-${BMINOR:-./bminor}}
RUN=${BMINOR_RUN:--interp}
RUNTIME=${BMINOR_RUNTIME:-build/library.o}
CC=${BMINOR_CC:-gcc-14}
[[ -x $BIN ]] || { echo "error: $BIN not built; run 'make' first"; exit 1; }
if [[ $RUN == -codegen && ! -f $RUNTIME ]]; then
    echo "error: runtime $RUNTIME not built; run 'make' first"; exit 1
fi

RED=$'\e[31m'; GREEN=$'\e[32m'; DIM=$'\e[2m'; RESET=$'\e[0m'
[[ -t 1 ]] || { RED=; GREEN=; DIM=; RESET=; }

pass=0; fail=0; failed=()
out=$(mktemp); err=$(mktemp); asm=$(mktemp --suffix=.s); exe=$(mktemp)
trap 'rm -f "$out" "$err" "$asm" "$exe"' EXIT

# Sets `status`, or `why` when the program could not be built.
run_program() {
    local file=$1
    why=""
    if [[ $RUN != -codegen ]]; then
        timeout 10 "$BIN" "$RUN" "$file" >"$out" 2>"$err"
        status=$?
        return
    fi

    if ! "$BIN" -codegen "$file" >"$asm" 2>"$err"; then
        why="compiler failed"; return
    fi
    if ! "$CC" -o "$exe" "$asm" "$RUNTIME" -lm 2>"$err"; then
        why="assemble/link failed"; return
    fi
    timeout 10 "$exe" >"$out" 2>"$err"
    status=$?
}

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

    run_program "$file"

    if [[ -n $why ]]; then
        :
    elif [[ $status -eq 124 ]]; then
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
