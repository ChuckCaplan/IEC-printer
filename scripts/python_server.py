#!/usr/bin/env python3
# TODO add comments
 
import subprocess
import socket
import time
import argparse
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
            # swallow an immediate CR/LF pair
            if i < n and raw[i] in (10, 13) and raw[i] != byte:
                i += 1
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

    # Find content bounds to trim and center
    non_bg_rows = np.any(canvas != bg, axis=1)
    non_bg_cols = np.any(canvas != bg, axis=0)

    if np.any(non_bg_rows):
        # Trim whitespace
        first_row = np.argmax(non_bg_rows)
        last_row = len(non_bg_rows) - np.argmax(np.flip(non_bg_rows))
        first_col = np.argmax(non_bg_cols)
        last_col = len(non_bg_cols) - np.argmax(np.flip(non_bg_cols))
        # Crop the canvas to the content, removing all whitespace
        canvas = canvas[first_row:last_row, first_col:last_col]
    else:
        # Image is empty, return a 1x1 pixel image
        canvas = np.full((1, 1), bg, dtype=np.uint8)

    return canvas

def parse_and_render2(raw, bg=255, fg=0):
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
    height = DOT_ROW_HEIGHT * len(lines)
    img = np.full((height, width), bg, np.uint8)

    y = 0
    for cols in lines:
        for x, col in enumerate(cols):
            for r in range(DOT_ROW_HEIGHT):
                if (col >> r) & 1:
                    img[y + r, x] = fg
        y += DOT_ROW_HEIGHT

    # Trim whitespace like parse_and_render does
    non_bg_rows = np.any(img != bg, axis=1)
    non_bg_cols = np.any(img != bg, axis=0)

    if np.any(non_bg_rows):
        first_row = np.argmax(non_bg_rows)
        last_row = len(non_bg_rows) - np.argmax(np.flip(non_bg_rows))
        first_col = np.argmax(non_bg_cols)
        last_col = len(non_bg_cols) - np.argmax(np.flip(non_bg_cols))
        img = img[first_row:last_row, first_col:last_col]
    else:
        img = np.full((1, 1), bg, dtype=np.uint8)

    return img

def scale_and_write_bmp(path, canvas, dpi=300, paper_width_in=8.5, paper_height_in=11, bg=255):
    """
    Scales the canvas to fit a standard paper size at a given DPI,
    preserving the aspect ratio. Splits tall images into multiple pages.
    Returns list of created file paths.
    """
    source_img = Image.fromarray(canvas, mode="L")
    page_width_px = int(paper_width_in * dpi)
    page_height_px = int(paper_height_in * dpi)
    source_width, source_height = source_img.size
    
    if source_width == 0 or source_height == 0:
        final_img = Image.new("L", (1, 1), bg)
        final_img.save(path, format="BMP")
        return [path]

    # Check if this is a tall banner by aspect ratio
    # Banners are extremely tall relative to width (height > 2x width)
    is_banner = source_height > (source_width * 2)
    
    # Scale to page width to see if it would span multiple pages
    scale_to_width = page_width_px / source_width
    height_at_full_width = int(source_height * scale_to_width)
    
    if is_banner and height_at_full_width > page_height_px:
        # Multi-page banner - scale to page width and split
        new_width = page_width_px
        new_height = height_at_full_width
        resized_img = source_img.resize((new_width, new_height), Image.Resampling.NEAREST)
        
        num_pages = (new_height + page_height_px - 1) // page_height_px
        base_path = path.rsplit('.', 1)[0]
        created_files = []
        
        for page_num in range(num_pages):
            y_start = page_num * page_height_px
            y_end = min(y_start + page_height_px, new_height)
            page_img = resized_img.crop((0, y_start, new_width, y_end))
            
            final_page = Image.new("L", (page_width_px, page_height_px), bg)
            final_page.paste(page_img, (0, 0))
            
            page_path = f"{base_path}_page{page_num + 1}.bmp"
            final_page.save(page_path, format="BMP")
            created_files.append(page_path)
        
        return created_files
    
    # Single page - scale to fit and center
    scale = min(page_width_px / source_width, page_height_px / source_height)
    new_width = int(source_width * scale)
    new_height = int(source_height * scale)
    resized_img = source_img.resize((new_width, new_height), Image.Resampling.NEAREST)
    final_img = Image.new("L", (page_width_px, page_height_px), bg)
    final_img.paste(resized_img, ((page_width_px - new_width) // 2, (page_height_px - new_height) // 2))
    final_img.save(path, format="BMP")
    return [path]
    
def analyze_raw_data(raw):
    """Suggests which parser to use based on data patterns."""
    has_repeat = 0x1A in raw  # repeat column
    has_esc = 0x1B in raw     # ESC sequences
    
    # Check for actual POS commands (0x10 followed by two ASCII digits)
    has_pos = False
    for i in range(len(raw) - 2):
        if raw[i] == 0x10:
            c1, c2 = raw[i+1], raw[i+2]
            if 48 <= c1 <= 57 and 48 <= c2 <= 57:
                has_pos = True
                break
    
    # Count 0x08 and 0x0D markers (method2 signature)
    count_08 = raw.count(0x08)
    count_0d = raw.count(0x0D)
    
    print(f"Data size: {len(raw)} bytes")
    print(f"Has repeat (0x1A): {has_repeat}")
    print(f"Has POS (0x10+digits): {has_pos}")
    print(f"Has ESC (0x1B): {has_esc}")
    print(f"Graphics markers (0x08): {count_08}")
    print(f"Line markers (0x0D): {count_0d}")
    
    # Use method2 if we have band markers and no complex commands
    if count_08 > 0 and count_0d > 0 and not (has_repeat or has_esc or has_pos):
        print("→ Using: parse_and_render2 (simple band format)")
        return parse_and_render2(raw)
    else:
        print("→ Using: parse_and_render (complex format)")
        return parse_and_render(raw)

def start_server(host='0.0.0.0', port=65432):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((host, port))
        s.listen()
        print(f"Server listening on {host}:{port}")
        while True:
            conn, addr = s.accept()
            all_data = bytearray()
            with conn:
                print(f"Connected by {addr}")
                # Loop to receive all data from the client in chunks
                while True:
                    data = conn.recv(4096) # Read up to 4096 bytes
                    if not data:
                        break # No more data, client has closed the connection
                    all_data.extend(data)

                if all_data:
                    canvas = analyze_raw_data(all_data)

                    out_bmp = f"{time.time()}.bmp"

                    try:
                        created_files = scale_and_write_bmp(out_bmp, canvas)
                        if len(created_files) == 1:
                            print(f"Wrote BMP: {created_files[0]} ({canvas.shape[1]}x{canvas.shape[0]})", file=sys.stderr)
                        else:
                            print(f"Wrote {len(created_files)} pages: {', '.join(created_files)}", file=sys.stderr)
                        # print(f"Printing: {' '.join(created_files)}", file=sys.stderr)
                        # subprocess.run(["lp"] + created_files, check=True, capture_output=True, text=True)
                    except Exception as e:
                        print(f"Failed to write or print BMP: {e}", file=sys.stderr)


if __name__ == "__main__":
    start_server()
