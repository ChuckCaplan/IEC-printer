#!/usr/bin/env python3
# mps803_render.py
# See top-of-file comment for details. (Same content as previously executed.)

import argparse
import os
import sys
import numpy as np
from PIL import Image

DOT = 7  # 7-pin MPS-803

def parse_and_render(raw):
    lines = []
    line = []
    bit = False
    i, n = 0, len(raw)

    while i < n:
        b = raw[i]; i += 1

        if b == 0x08:               # graphics mode ON for this band
            bit = True
            continue

        if b == 0x0D:               # CR ends the 7-dot band
            # compress any run of CRs into empty bands
            lines.append(line)
            line = []
            bit = False
            while i < n and raw[i] == 0x0D:
                lines.append([])    # extra blank bands
                i += 1
            continue

        if bit:
            line.append(b & 0x7F)   # **EVERY byte is one column; mask to 7 bits**

    if line:
        lines.append(line)

    # Render
    width = max((len(l) for l in lines), default=0)
    height = DOT * len(lines)
    img = np.full((height, width), 255, np.uint8)

    y = 0
    for cols in lines:
        for x, col in enumerate(cols):
            for r in range(DOT):
                if (col >> r) & 1:
                    img[y + r, x] = 0
        y += DOT

    return Image.fromarray(img, 'L')

def write_bmp(path, arr):
    arr.save(path, format="BMP")

def main():
    ap = argparse.ArgumentParser(description="Render Commodore MPS-803 raw byte stream to BMP.")
    ap.add_argument("input")
    ap.add_argument("--invert", action="store_true")
    args = ap.parse_args()

    in_path = args.input
    if not os.path.exists(in_path):
        print(f"Input not found: {in_path}", file=sys.stderr); sys.exit(1)

    with open(in_path, "rb") as f:
        raw = f.read()

    canvas = parse_and_render(raw)
    # The `canvas` is a PIL Image object, which does not have a `.shape` attribute.
    # We need to get its dimensions before potentially modifying it.
    width, height = canvas.size
    if args.invert:
        canvas = Image.fromarray(255 - np.array(canvas))

    base, _ = os.path.splitext(in_path)
    out_bmp = f"{base}.bmp"

    write_bmp(out_bmp, canvas)
    print(f"Wrote BMP: {out_bmp} ({width}x{height})", file=sys.stderr)

if __name__ == "__main__":
    main()
