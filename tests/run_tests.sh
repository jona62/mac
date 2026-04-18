#!/bin/bash
# Mac Language Test Runner
# Runs all .mac test files and compares output against // expect: annotations

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
MAC="$PROJECT_DIR/build/mac"

# Build first
echo "Building..."
cmake --build "$PROJECT_DIR/build" 2>&1 | tail -1
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

PASS=0
FAIL=0
TOTAL=0
BUILD_DIR="$PROJECT_DIR/build"

# Find all .mac files in test subdirectories (skip analyzer/ which has its own runner)
for test_file in $(find "$SCRIPT_DIR" -mindepth 2 -name "*.mac" -not -path "*/analyzer/*" | sort); do
    TOTAL=$((TOTAL + 1))
    rel_path="${test_file#$SCRIPT_DIR/}"

    # Extract expected output lines
    expected=""
    expect_error=""

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

    # Run the test from build dir so generated files stay out of the project root
    actual_stdout=$(cd "$BUILD_DIR" && "$MAC" "$test_file" 2>/tmp/mac_stderr)
    actual_stderr=$(cat /tmp/mac_stderr)

    # Compare stdout expectations
    passed=true

    if [ -n "$expected" ]; then
        # Remove trailing newline for comparison
        expected_trimmed=$(echo "$expected" | sed '/^$/d')
        actual_trimmed=$(echo "$actual_stdout" | sed '/^$/d')

        if [ "$expected_trimmed" != "$actual_trimmed" ]; then
            passed=false
        fi
    fi

    # Compare error expectations
    if [ -n "$expect_error" ]; then
        error_trimmed=$(echo "$expect_error" | sed '/^$/d')
        while IFS= read -r err_line; do
            if ! echo "$actual_stderr" | grep -qF "$err_line"; then
                passed=false
            fi
        done <<< "$error_trimmed"
    fi

    if $passed; then
        PASS=$((PASS + 1))
        echo "  PASS  $rel_path"
    else
        FAIL=$((FAIL + 1))
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
done

TOTAL=$((TOTAL + 1))
if bash "$SCRIPT_DIR/integration/path_resolution.sh" "$MAC"; then
    PASS=$((PASS + 1))
    echo "  PASS  integration/path_resolution.sh"
else
    FAIL=$((FAIL + 1))
    echo "  FAIL  integration/path_resolution.sh"
fi

echo ""
echo "Results: $PASS passed, $FAIL failed, $TOTAL total"

# Clean up generated test artifacts from build dir (only root-level images, not assets/)
find "$BUILD_DIR" -maxdepth 1 \( -name '*.gif' -o -name '*.png' -o -name '*.jpg' \) -delete 2>/dev/null

if [ $FAIL -gt 0 ]; then
    exit 1
fi
exit 0
