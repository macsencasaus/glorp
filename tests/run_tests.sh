#!/bin/bash

PASS_COUNT=0
FAIL_COUNT=0

for test_in in ./*.in; do
    test_name=$(basename "$test_in" .in)
    expected_out="./$test_name.out"

    if [[ ! -f "$expected_out" ]]; then
        echo "Missing expected out for $test_name"
        continue
    fi

    actual_out=$(mktemp)
    "./$test_in" > "$actual_out" 2>&1

    if diff -u "$expected_out" "$actual_out" > /dev/null; then
        echo "[PASS] $test_name"
        ((PASS_COUNT++))
    else
        echo "[FAIL] $test_name"
        diff -u "$expected_out" "$actual_out"
        ((FAIL_COUNT++))
    fi

    rm "$actual_out"
done

echo
echo "Passed: $PASS_COUNT"
echo "Failed: $FAIL_COUNT"

exit $FAIL_COUNT
