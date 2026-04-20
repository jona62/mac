#!/bin/bash
# Mac Language Test Runner
# Runs all .mac test files and compares output against // expect: annotations

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
MAC="$PROJECT_DIR/build/mac"
JOBS=${MAC_TEST_JOBS:-8}

# Build first
echo "Building..."
cmake --build "$PROJECT_DIR/build" 2>&1 | tail -1
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

BUILD_DIR="$PROJECT_DIR/build"
RESULTS_DIR=$(mktemp -d "${TMPDIR:-/tmp}/mac-test-results.XXXXXX")
trap 'rm -rf "$RESULTS_DIR"' EXIT

run_one_test() {
    local test_file="$1"
    local rel_path="${test_file#$SCRIPT_DIR/}"
    local result_file="$RESULTS_DIR/$(echo "$rel_path" | tr '/' '_')"

    # Extract expected output lines
    local expected="" expect_error=""
    while IFS= read -r line; do
        if echo "$line" | grep -q '// expect: '; then
            val=$(echo "$line" | sed 's/.*\/\/ expect: //')
            expected="${expected}${val}
"
        elif echo "$line" | grep -q '// expect runtime error: '; then
            val=$(echo "$line" | sed 's/.*\/\/ expect runtime error: //')
            expect_error="${expect_error}${val}
"
        elif echo "$line" | grep -q '// expect error: '; then
            val=$(echo "$line" | sed 's/.*\/\/ expect error: //')
            expect_error="${expect_error}${val}
"
        fi
    done < "$test_file"

    # Run the test
    local stderr_file="$RESULTS_DIR/stderr_$$_$RANDOM"
    local actual_stdout
    actual_stdout=$(cd "$BUILD_DIR" && "$MAC" "$test_file" 2>"$stderr_file")
    local actual_stderr
    actual_stderr=$(cat "$stderr_file")
    rm -f "$stderr_file"

    # Compare
    local passed=true

    if [ -n "$expected" ]; then
        local expected_trimmed actual_trimmed
        expected_trimmed=$(echo "$expected" | sed '/^$/d')
        actual_trimmed=$(echo "$actual_stdout" | sed '/^$/d')
        if [ "$expected_trimmed" != "$actual_trimmed" ]; then
            passed=false
        fi
    fi

    if [ -n "$expect_error" ]; then
        local error_trimmed
        error_trimmed=$(echo "$expect_error" | sed '/^$/d')
        while IFS= read -r err_line; do
            if ! echo "$actual_stderr" | grep -qF "$err_line"; then
                passed=false
            fi
        done <<< "$error_trimmed"
    fi

    if $passed; then
        echo "PASS" > "$result_file"
        echo "  PASS  $rel_path"
    else
        echo "FAIL" > "$result_file"
        echo "  FAIL  $rel_path"
        if [ -n "$expected" ] && [ "$expected_trimmed" != "$actual_trimmed" ]; then
            echo "        Expected stdout:"
            echo "$expected_trimmed" | sed 's/^/          /'
            echo "        Actual stdout:"
            echo "$actual_trimmed" | sed 's/^/          /'
        fi
        if [ -n "$expect_error" ]; then
            while IFS= read -r err_line; do
                if ! echo "$actual_stderr" | grep -qF "$err_line"; then
                    echo "        Missing error: $err_line"
                    echo "        Actual stderr: $actual_stderr"
                fi
            done <<< "$error_trimmed"
        fi
    fi
}

export -f run_one_test
export SCRIPT_DIR BUILD_DIR MAC RESULTS_DIR

# Run .mac tests in parallel
find "$SCRIPT_DIR" -mindepth 2 -name "*.mac" -not -path "*/analyzer/*" | sort | \
    xargs -P "$JOBS" -I {} bash -c 'run_one_test "$@"' _ {}

# Run integration tests sequentially (they use shared state)
for integration_test in "$SCRIPT_DIR"/integration/*.sh; do
    test_name="integration/$(basename "$integration_test")"
    if bash "$integration_test" "$MAC"; then
        echo "PASS" > "$RESULTS_DIR/$(echo "$test_name" | tr '/' '_')"
        echo "  PASS  $test_name"
    else
        echo "FAIL" > "$RESULTS_DIR/$(echo "$test_name" | tr '/' '_')"
        echo "  FAIL  $test_name"
    fi
done

# Count results
PASS=$(grep -rl "PASS" "$RESULTS_DIR" | wc -l | tr -d ' ')
FAIL=$(grep -rl "FAIL" "$RESULTS_DIR" | wc -l | tr -d ' ')
TOTAL=$((PASS + FAIL))

echo ""
echo "Results: $PASS passed, $FAIL failed, $TOTAL total"

# Clean up generated test artifacts
find "$BUILD_DIR" -maxdepth 1 \( -name '*.gif' -o -name '*.png' -o -name '*.jpg' \) -delete 2>/dev/null

if [ $FAIL -gt 0 ]; then
    exit 1
fi
exit 0
