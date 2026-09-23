#!/usr/bin/env bash
#
# B-Minor regression test driver.
#
# Layout convention:
#   test/<stage>/good*.bminor   must exit 0
#   test/<stage>/bad*.bminor    must exit non-zero
#   test/<stage>/<name>.expected   optional; if present, stdout must match it
#
# Stages run in compiler-pipeline order; a stage directory that does not exist
# yet is skipped, so this script works from Phase 1 onward without edits.
#
# Env:
#   BMINOR_VALGRIND=1   run each case under valgrind, failing on any leak/error
#   BMINOR_STAGES="..." run only the named stages

set -u

# Move to the project root directory
cd "$(dirname "$0")/.." || exit 1

BIN=./bminor

# Test if the binary exists and is executable
[[ -x $BIN ]] || { echo "error: $BIN not built; run 'make' first"; exit 1; }

# stage directory : driver flag
STAGES=(
    "scanner:-scan"
    "parser:-parse"
    "printer:-print"
    "resolver:-resolve"
    "typechecker:-typecheck"
)

# Color codes for output formatting
RED=$'\e[31m'; GREEN=$'\e[32m'; DIM=$'\e[2m'; RESET=$'\e[0m'
[[ -t 1 ]] || { RED=; GREEN=; DIM=; RESET=; }

pass=0; fail=0; failed_cases=()

# Output is captured to a file because command substitution strips
# trailing newlines, making byte-exact comparison impossible.
OUTFILE=$(mktemp)
trap 'rm -f "$OUTFILE"' EXIT

run_case() {
    local flag=$1 file=$2 expect=$3 name
    name=${file#test/} # strip leading "test/"

    local status
    if [[ ${BMINOR_VALGRIND:-0} == 1 ]]; then
        valgrind --quiet --error-exitcode=125 --leak-check=full \
                 --errors-for-leak-kinds=definite \
                 "$BIN" "$flag" "$file" >"$OUTFILE" 2>&1
    else
        "$BIN" "$flag" "$file" >"$OUTFILE" 2>&1
    fi
    status=$? # capture the exit status of the command

    if [[ $status -eq 125 && ${BMINOR_VALGRIND:-0} == 1 ]]; then
        # handle valgrind-specific exit code
        printf '%s FAIL%s %s %s(valgrind)%s\n' "$RED" "$RESET" "$name" "$DIM" "$RESET"
        sed 's/^/      /' "$OUTFILE"
        fail=$((fail+1)); failed_cases+=("$name"); return
    fi

    local ok=1 why=""
    if [[ $expect == good && $status -ne 0 ]]; then
        ok=0; why="expected success, exited $status"
    elif [[ $expect == bad && $status -eq 0 ]]; then
        ok=0; why="expected failure, exited 0"
    fi

    # Check expected output if an expected file exists
    local expected_file=${file%.bminor}.expected
    if [[ $ok -eq 1 && -f $expected_file ]]; then
        if ! diff -u "$expected_file" "$OUTFILE" >/dev/null; then
            ok=0; why="output mismatch"
        fi
    fi

    if [[ $ok -eq 1 ]]; then
        printf '%s pass%s %s\n' "$GREEN" "$RESET" "$name"
        pass=$((pass+1))
    else
        printf '%s FAIL%s %s %s(%s)%s\n' "$RED" "$RESET" "$name" "$DIM" "$why" "$RESET"
        if [[ $why == "output mismatch" ]]; then
            diff -u "$expected_file" "$OUTFILE" | tail -n +3 | sed 's/^/      /'
        else
            sed 's/^/      /' "$OUTFILE"
        fi
        fail=$((fail+1)); failed_cases+=("$name")
    fi
}

for entry in "${STAGES[@]}"; do
    stage=${entry%%:*}
    flag=${entry##*:}

    # Skip stages not listed in BMINOR_STAGES, if it is set
    if [[ -n ${BMINOR_STAGES:-} ]] && [[ " $BMINOR_STAGES " != *" $stage "* ]]; then
        continue
    fi

    # Use nullglob to skip cases where no test files exist for a stage
    shopt -s nullglob
    cases=(test/"$stage"/*.bminor)
    shopt -u nullglob
    [[ ${#cases[@]} -gt 0 ]] || continue

    # Run test cases for this stage
    printf '\n%s=== %s (%s) ===%s\n' "$DIM" "$stage" "$flag" "$RESET"
    for file in "${cases[@]}"; do
        base=$(basename "$file")
        case $base in
            good*) run_case "$flag" "$file" good ;;
            bad*)  run_case "$flag" "$file" bad ;;
            *)     printf '%s SKIP%s %s %s(name must start with good/bad)%s\n' \
                          "$RED" "$RESET" "$file" "$DIM" "$RESET" ;;
        esac
    done
done

# Test summary
printf '\n%d passed, %d failed\n' "$pass" "$fail"
if [[ $fail -gt 0 ]]; then
    printf 'failing: %s\n' "${failed_cases[*]}"
    exit 1
fi
exit 0
