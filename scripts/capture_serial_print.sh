#!/usr/bin/env bash
# capture_serial_print.sh
# Capture raw print data from a USB serial port until idle, write to print.dump,
# then call run_vice_print.sh with the dump file.
#
# Usage:
#   ./capture_serial_print.sh /dev/ttyUSB0
#   ./capture_serial_print.sh /dev/tty.usbserial-1410
#
# Env overrides:
#   RUN_SCRIPT=/path/to/run_vice_print.sh
#   OUTFILE=print.dump
#   BAUD=9600
#   IDLE_SECS=2          # consider job done after this many seconds of no new bytes
#   CHECK_INTERVAL=0.5   # seconds between size checks
#   REQUIRE_NONZERO=1    # don't finish until file has >0 bytes at least once

set -euo pipefail

PORT="${1:-}"
[[ -n "$PORT" ]] || { echo "Usage: $0 /dev/ttyUSB0|/dev/ttyACM0|/dev/tty.usbserial-XXXX" >&2; exit 1; }

RUN_SCRIPT="${RUN_SCRIPT:-./run_vice_print.sh}"
OUTFILE="${OUTFILE:-print.dump}"
BAUD="${BAUD:-9600}"
IDLE_SECS="${IDLE_SECS:-2}"
CHECK_INTERVAL="${CHECK_INTERVAL:-0.5}"
REQUIRE_NONZERO="${REQUIRE_NONZERO:-1}"

os="$(uname -s)"
stty_flag="-F"
case "$os" in
  Darwin) stty_flag="-f" ;;
esac

need() { command -v "$1" >/dev/null 2>&1; }
need stty
need cat

# Configure serial: raw 8N1, no flow control, desired baud
# Tweak h/w or s/w flow control if your adapter/printer needs it (remove -ixon/-ixoff/-crtscts).
echo "[serial] configuring $PORT at ${BAUD} baud (8N1, raw)"
stty "$stty_flag" "$PORT" "$BAUD" raw -echo -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten \
     -ixon -ixoff -crtscts cs8 -cstopb -parenb

# Fresh outfile
: > "$OUTFILE"

# Start capture
echo "[capture] writing to $OUTFILE (idle=${IDLE_SECS}s, check=${CHECK_INTERVAL}s)"
# Use redirection to avoid touching terminal settings for stdout
cat < "$PORT" >> "$OUTFILE" &
CAP_PID=$!

cleanup() {
  [[ -n "${CAP_PID:-}" ]] && kill "$CAP_PID" >/dev/null 2>&1 || true
}
trap cleanup EXIT

# Idle-detection loop
last_size=0
stable_for=0
saw_bytes=0

filesize() {
  # portable byte count
  wc -c < "$OUTFILE" 2>/dev/null | tr -d '[:space:]' || echo 0
}

while :; do
  sleep "$CHECK_INTERVAL"
  sz=$(filesize)

  if (( sz > 0 )); then
    saw_bytes=1
  fi

  if [[ "$sz" -eq "$last_size" ]]; then
    # no growth
    stable_for=$(awk -v a="$stable_for" -v b="$CHECK_INTERVAL" 'BEGIN{printf "%.3f", a+b}')
  else
    stable_for=0
    last_size="$sz"
  fi

  # finish if we’ve been idle long enough, and either nonzero bytes seen (default)
  # or nonzero not required
  if awk -v s="$stable_for" -v t="$IDLE_SECS" 'BEGIN{exit !(s>=t)}'; then
    if [[ "$REQUIRE_NONZERO" -eq 0 || "$saw_bytes" -eq 1 ]]; then
      break
    fi
  fi
done

echo "[capture] idle detected (${stable_for}s). Finalizing…"
kill "$CAP_PID" >/dev/null 2>&1 || true
wait "$CAP_PID" 2>/dev/null || true

final_size=$(filesize)
if (( final_size == 0 )); then
  echo "[capture] WARNING: $OUTFILE is empty; skipping pipeline." >&2
  exit 2
fi

echo "[capture] got $final_size bytes → $OUTFILE"
echo "[run] $RUN_SCRIPT $OUTFILE"
exec "$RUN_SCRIPT" "$OUTFILE"