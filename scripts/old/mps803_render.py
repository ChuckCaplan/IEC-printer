#!/usr/bin/env python3
# mps803_render.py
# See top-of-file comment for details. (Same content as previously executed.)

import argparse
import os
import sys
import numpy as np
from PIL import Image

DOT_ROW_HEIGHT = 7  # vertical dots per printed "band"

def ensure_height(canvas, height, width, bg):
    if canvas.shape[0] >= height:
        return canvas
    add_rows = height - canvas.shape[0]
    extra = np.full((add_rows, width), bg, dtype=np.uint8)
    return np.vstack([canvas, extra])

def plot_column(canvas, x, y, pattern, fg, width, bg):
    patt = pattern & 0x7F
    if x < 0 or x >= width:
        return canvas
    canvas = ensure_height(canvas, y + DOT_ROW_HEIGHT, width, bg)
    for b in range(DOT_ROW_HEIGHT):
        if (patt >> b) & 1:
            canvas[y + b, x] = fg
    return canvas

def parse_and_render(raw, width=640, bg=255, fg=0):
    canvas = np.full((1, width), bg, dtype=np.uint8)

    x = 0
    y = 0
    i = 0
    n = len(raw)
    bit_image = False

    def newline():
        nonlocal x, y, canvas
        x = 0
        y += DOT_ROW_HEIGHT
        canvas = ensure_height(canvas, y + DOT_ROW_HEIGHT, width, bg)

    while i < n:
        byte = raw[i]; i += 1

        if byte == 8:  # bit image mode
            bit_image = True
            continue

        if byte == 26 and bit_image:  # repeat column
            if i + 2 <= n:
                count = raw[i]; patt = raw[i+1]; i += 2
                count = 256 if count == 0 else count
                for _ in range(count):
                    if x >= width:
                        newline()
                    canvas = plot_column(canvas, x, y, patt, fg, width, bg)
                    x += 1
            continue

        if byte == 16 and i + 2 <= n:  # POS two ASCII digits
            c1, c2 = raw[i], raw[i+1]
            if 48 <= c1 <= 57 and 48 <= c2 <= 57:
                pos = (c1 - 48) * 10 + (c2 - 48)
                x = min(width - 1, pos * 6)
            i += 2
            continue

        if byte == 27:  # ESC
            if i < n and raw[i] == 16 and i + 3 <= n:
                i += 1
                nH = raw[i]; nL = raw[i+1]; i += 2
                addr = (nH << 8) | nL
                if addr >= width:
                    lines_down = addr // width
                    for _ in range(lines_down):
                        newline()
                    addr = addr % width
                x = addr
            continue

        if byte in (10, 13):  # LF/CR
            newline()
            continue

        if byte in (14, 15):  # enhance off/on cancels bit image
            bit_image = False
            continue

        if byte in (17, 145, 18, 146, 34):  # ignored modes
            continue

        if bit_image:
            if x >= width:
                newline()
            canvas = plot_column(canvas, x, y, byte, fg, width, bg)
            x += 1

    # Trim trailing bg rows
    last_non_bg = 0
    for r in range(canvas.shape[0] - 1, -1, -1):
        if np.any(canvas[r] != bg):
            last_non_bg = r + 1
            break
    canvas = canvas[:max(1, last_non_bg), :]
    return canvas

def write_ppm(path, arr):
    h, w = arr.shape
    with open(path, "wb") as f:
        f.write(f"P6\n{w} {h}\n255\n".encode("ascii"))
        rgb = np.repeat(arr[:, :, None], 3, axis=2).astype(np.uint8)
        f.write(rgb.tobytes())

def write_bmp(path, arr):
    img = Image.fromarray(arr, mode="L")
    img.save(path, format="BMP")

def main():
    ap = argparse.ArgumentParser(description="Render Commodore MPS-803 raw byte stream to PPM/BMP.")
    ap.add_argument("input")
    ap.add_argument("--width", type=int, default=640)
    ap.add_argument("--bg", type=int, default=255)
    ap.add_argument("--fg", type=int, default=0)
    ap.add_argument("--invert", action="store_true")
    ap.add_argument("--ppm", help="Output PPM path (default: <input>.ppm)")
    ap.add_argument("--bmp", help="Output BMP path (default: <input>.bmp)")
    ap.add_argument("--ppm-only", action="store_true")
    ap.add_argument("--bmp-only", action="store_true")
    args = ap.parse_args()

    in_path = args.input
    if not os.path.exists(in_path):
        print(f"Input not found: {in_path}", file=sys.stderr); sys.exit(1)

    with open(in_path, "rb") as f:
        raw = f.read()

    canvas = parse_and_render(raw, width=args.width, bg=args.bg, fg=args.fg)
    if args.invert:
        canvas = 255 - canvas

    base, _ = os.path.splitext(in_path)
    out_ppm = args.ppm or f"{base}.ppm"
    out_bmp = args.bmp or f"{base}.bmp"

    # Which to write
    write_ppm_flag = not args.bmp_only
    write_bmp_flag = not args.ppm_only

    if write_ppm_flag:
        write_ppm(out_ppm, canvas)
        print(f"Wrote PPM: {out_ppm} ({canvas.shape[1]}x{canvas.shape[0]})", file=sys.stderr)

    if write_bmp_flag:
        try:
            write_bmp(out_bmp, canvas)
            print(f"Wrote BMP: {out_bmp} ({canvas.shape[1]}x{canvas.shape[0]})", file=sys.stderr)
        except Exception as e:
            print(f"Failed to write BMP: {e}", file=sys.stderr)

if __name__ == "__main__":
    main()
