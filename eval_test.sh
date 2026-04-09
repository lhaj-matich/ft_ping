#!/bin/bash

BIN=./ft_ping
REAL_PING=ping

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS() { echo -e "${GREEN}✔ $1${NC}"; }
FAIL() { echo -e "${RED}✘ $1${NC}"; }
INFO() { echo -e "${YELLOW}➜ $1${NC}"; }

TMP1=$(mktemp)
TMP2=$(mktemp)

cleanup() {
    rm -f "$TMP1" "$TMP2"
}
trap cleanup EXIT

echo "=============================="
echo "🔥 FT_PING TEST SUITE"
echo "=============================="

# -----------------------------
# 1. Binary exists
# -----------------------------
[ -f "$BIN" ] && PASS "Binary exists" || { FAIL "Binary missing"; exit 1; }

# -----------------------------
# 2. Help flag
# -----------------------------
$BIN -h > "$TMP1" 2>&1
grep -qi "usage" "$TMP1" && PASS "-h displays usage" || FAIL "-h failed"

# -----------------------------
# 3. Good IP test
# -----------------------------
IP="8.8.8.8"

INFO "Testing good IP ($IP)..."

$REAL_PING -c 3 $IP > "$TMP1" 2>&1 &
PID1=$!
sleep 3
kill -INT $PID1 2>/dev/null

$BIN $IP > "$TMP2" 2>&1 &
PID2=$!
sleep 3
kill -INT $PID2 2>/dev/null

diff <(head -n -1 "$TMP1") <(head -n -1 "$TMP2") >/dev/null \
    && PASS "Good IP behaves like ping" \
    || FAIL "Good IP mismatch"

# -----------------------------
# 4. Bad IP test
# -----------------------------
BAD_IP="10.255.255.1"

INFO "Testing bad IP..."

$REAL_PING -c 2 $BAD_IP > "$TMP1" 2>&1
$BIN $BAD_IP > "$TMP2" 2>&1

diff "$TMP1" "$TMP2" >/dev/null \
    && PASS "Bad IP handled correctly" \
    || FAIL "Bad IP mismatch"

# -----------------------------
# 5. -v bad IP
# -----------------------------
INFO "Testing -v with bad IP..."

$BIN -v $BAD_IP > "$TMP2" 2>&1

grep -qi "ttl\|icmp" "$TMP2" \
    && PASS "-v displays debug info" \
    || FAIL "-v missing verbose output"

# -----------------------------
# 6. Hostname test
# -----------------------------
HOST="google.com"

INFO "Testing hostname ($HOST)..."

$REAL_PING -c 3 $HOST > "$TMP1" 2>&1 &
PID1=$!
sleep 3
kill -INT $PID1 2>/dev/null

$BIN $HOST > "$TMP2" 2>&1 &
PID2=$!
sleep 3
kill -INT $PID2 2>/dev/null

diff <(head -n -1 "$TMP1") <(head -n -1 "$TMP2") >/dev/null \
    && PASS "Hostname works correctly" \
    || FAIL "Hostname mismatch"

# -----------------------------
# 7. Bad hostname
# -----------------------------
BAD_HOST="noexist.abc"

INFO "Testing bad hostname..."

$BIN $BAD_HOST > "$TMP2" 2>&1

grep -qi "unknown\|error" "$TMP2" \
    && PASS "Bad hostname handled" \
    || FAIL "Bad hostname not handled"

# -----------------------------
# 8. CTRL+C behavior
# -----------------------------
INFO "Testing CTRL+C handling..."

$BIN 8.8.8.8 > "$TMP2" 2>&1 &
PID=$!
sleep 2
kill -INT $PID
wait $PID 2>/dev/null

grep -qi "packets transmitted" "$TMP2" \
    && PASS "CTRL+C prints stats" \
    || FAIL "CTRL+C stats missing"

# -----------------------------
# 9. Memory leaks (macOS)
# -----------------------------
INFO "Checking memory leaks..."

leaks --atExit -- $BIN 8.8.8.8 > "$TMP2" 2>&1

grep -qi "0 leaks" "$TMP2" \
    && PASS "No memory leaks" \
    || FAIL "Memory leaks detected"

echo "=============================="
echo "✅ TESTING COMPLETE"
echo "=============================="
