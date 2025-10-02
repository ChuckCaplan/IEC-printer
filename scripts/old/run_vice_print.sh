#!/usr/bin/env bash
# run_vice_print.sh
# Runs the C64 toolchain end-to-end and reports total runtime.

set -euo pipefail

### ---- Configuration (edit if you want different filenames/paths) ----
DISK_NAME="printimage"   # base name for D64 image
DISK_IMG="${DISK_NAME}.d64"
SEQ_IN="${1:-print.dump}" # Use first argument or default to print.dump
SEQ_NAME_ON_DISK='dump.seq,s'   # how it appears on the D64
BASIC_BAS="basic.bas"        # source BASIC file (if no basic.prg)
BASIC_PRG="basic.prg"
MON_CMDS="quit-when-done.mon"

# If your VICE emulator binary is named differently (e.g., x64sc-sdl2), change here:
X64_CMD="${X64_CMD:-x64sc}"     # allows override: X64_CMD=x64sc-sdl2 ./run_vice_print.sh

### ---- Dependency checks ----
need() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Error: '$1' not found in PATH." >&2
    exit 1
  }
}

need c1541
need "${X64_CMD}"
need petcat
need lp

# Input file checks
[[ -f "$SEQ_IN" ]]      || { echo "Error: Missing $SEQ_IN"; exit 1; }
if [[ ! -f "$BASIC_PRG" && ! -f $BASIC_BAS ]]; then
  echo "Error: Need either $BASIC_PRG or $BASIC_BAS to continue." >&2
  exit 1
fi
[[ -f "$MON_CMDS" ]]    || { echo "Error: Missing $MON_CMDS"; exit 1; }

# Time start (portable on macOS & Linux)
START_TS="$(date +%s)"

echo "==> Step 1: Build D64 and write $SEQ_IN as \"$SEQ_NAME_ON_DISK\""
# If an old image or BMP exists, remove it so format is clean
rm -f "$DISK_IMG"
rm -f *.bmp
c1541 -format ${DISK_NAME},00 d64 "$DISK_IMG" -attach "$DISK_IMG" -write "$SEQ_IN" "$SEQ_NAME_ON_DISK"

echo "==> Step 1.5: Ensure BASIC program exists"
if [[ ! -f "$BASIC_PRG" ]]; then
  echo "basic.prg not found, generating from basic.bas via petcat..."
  petcat -w2 -o "$BASIC_PRG" -- basic.bas
fi

echo "==> Step 2: Add BASIC program $BASIC_PRG to disk"
c1541 -attach "$DISK_IMG" -write "$BASIC_PRG"

echo "==> Step 3: Run emulator headless-ish with remote monitor and scripted quit"
# Notes:
#  - -console enables console I/O in some builds.
#  - -remotemonitor opens the monitor server; -moncommands feeds commands from file.
#  - -warp speeds execution; your $MON_CMDS should eventually issue 'quit'.
"${X64_CMD}" -console -remotemonitor -warp -moncommands "$MON_CMDS" "$DISK_IMG"

echo "==> Step 4: Print BMP(s) via lp"

# Find BMPs (case-insensitive) in current directory
shopt -s nullglob nocaseglob
bmps=( *.bmp )
shopt -u nocaseglob

count=${#bmps[@]}

if (( count == 0 )); then
  echo "Error: No BMP files found for printing" >&2
  exit 1
fi

echo "Found $count BMP file(s): ${bmps[*]}"
# leave commented out for now to avoid accidental printing, umcomment when ready for production
#lp "${bmps[@]}"

# Time end and report
END_TS="$(date +%s)"
ELAPSED=$(( END_TS - START_TS ))

# Simple human formatting
printf "==> All done. Total elapsed time: %s\n" "$( \
  awk -v s="$ELAPSED" 'BEGIN{
    h=int(s/3600); s-=h*3600;
    m=int(s/60);  s-=m*60;
    if (h>0) printf "%dh %dm %ds\n", h, m, s;
    else if (m>0) printf "%dm %ds\n", m, s;
    else printf "%ds\n", s;
  }'
)"