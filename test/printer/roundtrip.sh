#!/usr/bin/env bash
#
# Printer round-trip (idempotence) check.
#
# Property: printing a parsed program must produce valid B-Minor, and printing
# that output again must be byte-identical. This validates the printer without
# hard-coding a formatting style -- any precedence loss, dropped statement, or
# unescaped literal breaks one of the two steps.
#
# Usage: test/printer/roundtrip.sh

set -u

cd "$(dirname "$0")/../.." || exit 1

BIN=./bminor
[[ -x $BIN ]] || { echo "error: $BIN not built; run 'make' first"; exit 1; }

RED=$'\e[31m'; GREEN=$'\e[32m'; DIM=$'\e[2m'; RESET=$'\e[0m'
[[ -t 1 ]] || { RED=; GREEN=; DIM=; RESET=; }

pass=0; fail=0
tmp1=$(mktemp); tmp2=$(mktemp)
trap 'rm -f "$tmp1" "$tmp2"' EXIT

shopt -s nullglob
cases=(test/printer/good/*.bminor)
shopt -u nullglob
[[ ${#cases[@]} -gt 0 ]] || { echo "no printer fixtures found"; exit 0; }

printf '\n%s=== printer round-trip ===%s\n' "$DIM" "$RESET"

for file in "${cases[@]}"; do
    name=${file#test/}

    if ! "$BIN" -print "$file" >"$tmp1" 2>&1; then
        printf '%s FAIL%s %s %s(first print failed)%s\n' "$RED" "$RESET" "$name" "$DIM" "$RESET"
        sed 's/^/      /' "$tmp1"
        fail=$((fail+1)); continue
    fi

    # Reparsing its own output is what proves the printer emits valid B-Minor.
    if ! "$BIN" -print "$tmp1" >"$tmp2" 2>&1; then
        printf '%s FAIL%s %s %s(output is not valid B-Minor)%s\n' "$RED" "$RESET" "$name" "$DIM" "$RESET"
        sed 's/^/      /' "$tmp2"
        fail=$((fail+1)); continue
    fi

    if ! diff -u "$tmp1" "$tmp2" >/dev/null; then
        printf '%s FAIL%s %s %s(not idempotent)%s\n' "$RED" "$RESET" "$name" "$DIM" "$RESET"
        diff -u "$tmp1" "$tmp2" | tail -n +3 | sed 's/^/      /'
        fail=$((fail+1)); continue
    fi

    printf '%s pass%s %s\n' "$GREEN" "$RESET" "$name"
    pass=$((pass+1))
done

printf '\n%d passed, %d failed\n' "$pass" "$fail"
[[ $fail -eq 0 ]]
