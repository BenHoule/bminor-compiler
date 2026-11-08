#!/usr/bin/env bash
#
# B-Minor regression test driver.
#
# Layout convention:
#   test/<stage>/good/*.bminor   must exit 0
#   test/<stage>/bad/*.bminor    must exit non-zero
#   test/<stage>/{good,bad}/expected/<name>.expected is optional
#   test/<stage>/{good,bad}/out/<name>.out stores the program output
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

BIN=${BMINOR:-./bminor}

# Test if the binary exists and is executable
[[ -x $BIN ]] || { echo "error: $BIN not built; run 'make' first"; exit 1; }

# stage directory : driver flag
STAGES=(
    "scanner:-scan"
    "parser:-parse"
    "printer:-print"
    "resolver:-resolve"
    "typechecker:-typecheck"
    "optimizer:-O -ir"
)

# Color codes for output formatting
RED=$'\e[31m'; GREEN=$'\e[32m'; DIM=$'\e[2m'; RESET=$'\e[0m'
[[ -t 1 ]] || { RED=; GREEN=; DIM=; RESET=; }

pass=0; fail=0; failed_cases=()

run_case() {
    local flag=$1 file=$2 expect=$3 name case_dir test_name output_dir output_file expected_file
    name=${file#test/} # strip leading "test/"
    case_dir=$(dirname "$file")
    test_name=$(basename "$file" .bminor)
    output_dir="$case_dir/out"
    output_file="$output_dir/$test_name.out"
    expected_file="$case_dir/expected/$test_name.expected"

    if ! mkdir -p "$output_dir"; then
        printf '%s FAIL%s %s %s(could not create output directory)%s\n' \
            "$RED" "$RESET" "$name" "$DIM" "$RESET"
        fail=$((fail+1)); failed_cases+=("$name"); return
    fi

    local status
    if [[ ${BMINOR_VALGRIND:-0} == 1 ]]; then
        valgrind --quiet --error-exitcode=125 --leak-check=full \
                 --errors-for-leak-kinds=definite \
                 "$BIN" $flag "$file" >"$output_file" 2>&1
    else
        "$BIN" $flag "$file" >"$output_file" 2>&1
    fi
    status=$? # capture the exit status of the command

    if [[ $status -eq 125 && ${BMINOR_VALGRIND:-0} == 1 ]]; then
        # handle valgrind-specific exit code
        printf '%s FAIL%s %s %s(valgrind)%s\n' "$RED" "$RESET" "$name" "$DIM" "$RESET"
        sed 's/^/      /' "$output_file"
        fail=$((fail+1)); failed_cases+=("$name"); return
    fi

    local ok=1 why=""
    if [[ $expect == good && $status -ne 0 ]]; then
        ok=0; why="expected success, exited $status"
    elif [[ $expect == bad && $status -ne 1 ]]; then
        ok=0; why="expected compile error (1), exited $status"
    fi

    # Check expected output if an expected file exists
    if [[ $ok -eq 1 && -f $expected_file ]]; then
        if ! diff -u "$expected_file" "$output_file" >/dev/null; then
            ok=0; why="output mismatch"
        fi
    fi

    if [[ $ok -eq 1 ]]; then
        printf '%s pass%s %s\n' "$GREEN" "$RESET" "$name"
        pass=$((pass+1))
    else
        printf '%s FAIL%s %s %s(%s)%s\n' "$RED" "$RESET" "$name" "$DIM" "$why" "$RESET"
        if [[ $why == "output mismatch" ]]; then
            diff -u "$expected_file" "$output_file" | tail -n +3 | sed 's/^/      /'
        else
            sed 's/^/      /' "$output_file"
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

    shopt -s nullglob
    good_cases=(test/"$stage"/good/*.bminor)
    bad_cases=(test/"$stage"/bad/*.bminor)
    shopt -u nullglob
    [[ ${#good_cases[@]} -gt 0 || ${#bad_cases[@]} -gt 0 ]] || continue

    printf '\n%s=== %s (%s) ===%s\n' "$DIM" "$stage" "$flag" "$RESET"
    for file in "${good_cases[@]}"; do
        run_case "$flag" "$file" good
    done
    for file in "${bad_cases[@]}"; do
        run_case "$flag" "$file" bad
    done
done

# Test summary
printf '\n%d passed, %d failed\n' "$pass" "$fail"
if [[ $fail -gt 0 ]]; then
    printf 'failing: %s\n' "${failed_cases[*]}"
    exit 1
fi
exit 0
