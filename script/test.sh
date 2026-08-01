#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/cmake-build-debug-gcc-trunk"
SERVER_BIN="$BUILD_DIR/game-server-dev"
MANAGER_BIN="$BUILD_DIR/manager"
SERVER_LOG="$ROOT_DIR/logs/test-server.log"

TEST_FILES=(
  "$ROOT_DIR/tests/test_files/test_login"
  "$ROOT_DIR/tests/test_files/test_room"
  "$ROOT_DIR/tests/test_files/test_chat"
  "$ROOT_DIR/tests/test_files/test_match"
  "$ROOT_DIR/tests/test_files/test_reconnect"
)

mkdir -p "$ROOT_DIR/logs"

if [[ ! -x "$SERVER_BIN" ]]; then
  echo "missing executable: $SERVER_BIN" >&2
  exit 1
fi

if [[ ! -x "$MANAGER_BIN" ]]; then
  echo "missing executable: $MANAGER_BIN" >&2
  exit 1
fi

server_pid=""
cleanup() {
  if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT

echo "starting server..."
LD_LIBRARY_PATH="/home/hexne/gcc-trunk/lib64:${LD_LIBRARY_PATH:-}" \
  "$SERVER_BIN" >"$SERVER_LOG" 2>&1 &
server_pid=$!

sleep 1

for test_file in "${TEST_FILES[@]}"; do
  echo
  echo "==> ${test_file##*/}"
  "$MANAGER_BIN" "$test_file"
done

echo
echo "all tests finished"
