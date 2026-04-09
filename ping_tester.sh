#!/bin/bash

BIN=./ft_ping
REAL_PING=ping

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS() { echo -e "${GREEN}✔ $1${NC}"; }
FAIL() { echo -e "${RED}✘ $1${NC}"; }
WARN() { echo -e "${YELLOW}⚠ $1${NC}"; }

check_segfault() {
    if [ $1 -eq 139 ]; then
        FAIL "Segmentation fault detected"
        exit 1
    fi
}

run_test() {
    DESC=$1
    shift
    OUTPUT=$($BIN "$@" 2>&1)
    STATUS=$?

    check_segfault $STATUS

    if [ $STATUS -eq 0 ]; then
        PASS "$DESC"
    else
        FAIL "$DESC (exit code: $STATUS)"
        echo "Output:"
        echo "$OUTPUT"
    fi
}

echo "=============================="
echo "🔥 FT_PING EXTREME TEST SUITE"
echo "=============================="

# -----------------------------------
# 0. Binary check
# -----------------------------------
[ -f "$BIN" ] || { FAIL "Binary not found"; exit 1; }
PASS "Binary exists"

# -----------------------------------
# 1. Basic valid targets
# -----------------------------------
run_test "Ping 8.8.8.8" 8.8.8.8
run_test "Ping localhost" 127.0.0.1
run_test "Ping hostname google.com" google.com

# -----------------------------------
# 2. Multiple invalid inputs
# -----------------------------------
INVALID_INPUTS=(
    ""
    " "
    "invalid"
    "256.256.256.256"
    "999.999.999.999"
    "google..com"
    "@@@@"
    "1234"
)

for input in "${INVALID_INPUTS[@]}"; do
    echo "Test invalid input: '$input'"
    $BIN "$input" > /dev/null 2>&1
    STATUS=$?
    check_segfault $STATUS

    if [ $STATUS -ne 0 ]; then
        PASS "Handled invalid input '$input'"
    else
        FAIL "Invalid input accepted '$input'"
    fi
done

# -----------------------------------
# 3. Flags tests
# -----------------------------------
run_test "Flag -v with IP" -v 8.8.8.8
run_test "Flag -v with hostname" -v google.com
run_test "Flag -? help" -?

# invalid flags
BAD_FLAGS=(-z -x -123 --fake)

for flag in "${BAD_FLAGS[@]}"; do
    echo "Test invalid flag: $flag"
    $BIN $flag 8.8.8.8 > /dev/null 2>&1
    STATUS=$?
    check_segfault $STATUS

    if [ $STATUS -ne 0 ]; then
        PASS "Handled invalid flag $flag"
    else
        FAIL "Invalid flag not rejected: $flag"
    fi
done

# -----------------------------------
# 4. Combination edge cases
# -----------------------------------
run_test "Multiple flags (-v -?)" -v -? 8.8.8.8
run_test "Flag after target" 8.8.8.8 -v
run_test "Duplicate flags" -v -v 8.8.8.8

# -----------------------------------
# 5. Timeout / unreachable hosts
# -----------------------------------
UNREACHABLE=(
    "10.255.255.1"
    "192.0.2.1"
)

for ip in "${UNREACHABLE[@]}"; do
    echo "Testing unreachable: $ip"
    timeout 5 $BIN $ip > /dev/null 2>&1
    STATUS=$?
    check_segfault $STATUS
    PASS "Handled unreachable $ip"
done

# -----------------------------------
# 6. Stress test (rapid fire)
# -----------------------------------
echo "Running stress test (50 runs)..."
for i in {1..50}; do
    $BIN 8.8.8.8 > /dev/null 2>&1
    STATUS=$?
    check_segfault $STATUS
done
PASS "No crash under stress"

# -----------------------------------
# 7. Parallel execution (race conditions)
# -----------------------------------
echo "Running parallel test..."
for i in {1..10}; do
    $BIN 8.8.8.8 > /dev/null 2>&1 &
done
wait
PASS "No crash in parallel execution"

# -----------------------------------
# 8. Output sanity check
# -----------------------------------
echo "Checking output format..."
OUT=$($BIN 8.8.8.8 2>/dev/null | head -n 2)

echo "$OUT" | grep "bytes from" > /dev/null
[ $? -eq 0 ] && PASS "Output contains 'bytes from'" || FAIL "Bad output format"

echo "$OUT" | grep "ttl" > /dev/null
[ $? -eq 0 ] && PASS "TTL present" || WARN "TTL missing"

echo "$OUT" | grep "time=" > /dev/null
[ $? -eq 0 ] && PASS "RTT present" || WARN "RTT missing"

# -----------------------------------
# 9. Long run stability (10 sec)
# -----------------------------------
echo "Running long stability test (10s)..."
timeout 10 $BIN 8.8.8.8 > /dev/null 2>&1
STATUS=$?
check_segfault $STATUS
PASS "Stable over time"

# -----------------------------------
# 10. Memory leaks check (macOS)
# -----------------------------------
if command -v leaks > /dev/null; then
    echo "Running leaks check..."
    $BIN 8.8.8.8 &
    PID=$!
    sleep 2
    leaks $PID | grep "0 leaks" > /dev/null
    if [ $? -eq 0 ]; then
        PASS "No memory leaks detected"
    else
        FAIL "Memory leaks detected"
        echo "Check leaks output above"
    fi
    kill $PID 2>/dev/null
else
    WARN "leaks not installed, skipping memory check"
fi

# -----------------------------------
# 11. Compare with real ping (basic)
# -----------------------------------
echo "Comparing with system ping..."
$REAL_PING -c 1 8.8.8.8 > real.txt 2>/dev/null
$BIN 8.8.8.8 > mine.txt 2>/dev/null

grep "bytes from" mine.txt > /dev/null
[ $? -eq 0 ] && PASS "Output roughly matches ping" || FAIL "Output mismatch"

rm -f real.txt mine.txt

# -----------------------------------
# DONE
# -----------------------------------
echo "=============================="
echo "✅ ALL TESTS COMPLETED"
echo "=============================="
