#!/usr/bin/env python3

# Receives raw print data over TCP from a Commodore 64 via an Arduino Uno R4 WiFi (or similar), 
# converts to BMP, and optionally prints it.

# TODO:
# - Handle text mode, including PETSCII art via PETSCII TTF
# - Handle text printer commands (bold, underline, etc.)
# - Add HTTP server mode to serve latest image or status page
# - Add option to save raw data to a file for later analysis
 
import subprocess
import socket
import time
import sys
import argparse
import numpy as np
from PIL import Image

DOT_ROW_HEIGHT = 7  # vertical dots per printed band (MPS-803)

def parse_and_render(raw: bytes, width: int = 640, bg: int = 255, fg: int = 0):
    canvas = np.full((1, width), bg, dtype=np.uint8)

    def ensure_height(h):
        nonlocal canvas
        if canvas.shape[0] < h:
            add = h - canvas.shape[0]
            canvas = np.vstack([canvas, np.full((add, width), bg, dtype=np.uint8)])

    def plot_col(x, y, byte):
        # LSB at top, 7 dots tall
        if 0 <= x < width:
            ensure_height(y + DOT_ROW_HEIGHT)
            patt = byte & 0x7F
            for b in range(DOT_ROW_HEIGHT):
                if (patt >> b) & 1:
                    canvas[y + b, x] = fg

    x = 0
    y = 0
    i = 0
    n = len(raw)
    bit_image = False

    def newline():
        nonlocal x, y, bit_image
        x = 0
        y += DOT_ROW_HEIGHT
        ensure_height(y + DOT_ROW_HEIGHT)
        # The real printer leaves graphics mode between lines; you must send 0x08 again.
        bit_image = False

    while i < n:
        b = raw[i]; i += 1

        # Enter bit-image
        if b == 0x08:
            bit_image = True
            continue

        # Paper advance: the ONLY time we move to the next band
        if b in (0x0A, 0x0D):  # LF/CR
            # swallow CRLF pair
            if i < n and raw[i] in (0x0A, 0x0D) and raw[i] != b:
                i += 1
            newline()
            continue

        # Repeat columns: 0x1A, count, byte (count 0 => 256)
        if b == 0x1A:
            if i + 2 <= n:
                count = raw[i]; patt = raw[i+1]; i += 2
                count = 256 if count == 0 else count
                if bit_image:
                    # draw up to right margin; printer ignores the rest
                    todo = min(count, max(0, width - x))
                    for _ in range(todo):
                        plot_col(x, y, patt)
                        x += 1
                    # consume (but don’t draw) any overflow past 639, to keep X in sync
                    x += max(0, count - todo)
            continue

        # Everything else
        if bit_image:
            # In graphics mode **every byte is a column** (including 0x0E,0x0F,0x11,0x91,0x92,0x22, etc.)
            if x < width:
                plot_col(x, y, b)
            x += 1  # keep X in sync even if we clipped
        else:
            # Text mode: we don’t render glyphs; we only maintain carriage position
            # Standard cell ≈ 6 dots; Enhanced doubles width (you can wire that in if you track it)
            if 32 <= b <= 126:
                x = min(width - 1, x + 6)
            elif b == 9:  # TAB every 8 chars -> 48 dots
                x = min(width - 1, ((x // 48) + 1) * 48)
            # other controls (SO/SI/reverse/quote/etc.) don’t affect dots here

    # Trim whitespace border
    non_bg_rows = np.any(canvas != bg, axis=1)
    non_bg_cols = np.any(canvas != bg, axis=0)
    if np.any(non_bg_rows):
        r0 = np.argmax(non_bg_rows)
        r1 = len(non_bg_rows) - np.argmax(non_bg_rows[::-1])
        c0 = np.argmax(non_bg_cols)
        c1 = len(non_bg_cols) - np.argmax(non_bg_cols[::-1])
        canvas = canvas[r0:r1, c0:c1]
    else:
        canvas = np.full((1, 1), bg, dtype=np.uint8)

    return canvas

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
    
def start_server(host='0.0.0.0', port=65432, should_print=False):
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
                    canvas = parse_and_render(all_data)

                    out_bmp = f"{time.time()}.bmp"
                    
                    # create a dir called "output" if it doesn't exist
                    subprocess.run(["mkdir", "-p", "output"], check=True, capture_output=True, text=True)
                    out_bmp = "output/" + out_bmp

                    try:
                        created_files = scale_and_write_bmp(out_bmp, canvas)
                        if len(created_files) == 1:
                            print(f"Wrote BMP: {created_files[0]} ({canvas.shape[1]}x{canvas.shape[0]})", file=sys.stderr)
                        else:
                            print(f"Wrote {len(created_files)} pages: {', '.join(created_files)}", file=sys.stderr)
                        
                        if should_print:
                            print(f"Printing: {' '.join(created_files)}", file=sys.stderr)
                            subprocess.run(["lp"] + created_files, check=True, capture_output=True, text=True)

                    except Exception as e:
                        print(f"Failed to write or print BMP: {e}", file=sys.stderr)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Receives raw C64 print data, converts to BMP, and optionally prints.")
    parser.add_argument("-p", "--print", action="store_true", help="Print the generated BMP file(s) using 'lp'.")
    parser.add_argument("--host", default="0.0.0.0", help="Host for the server to listen on.")
    parser.add_argument("--port", type=int, default=65432, help="Port for the server to listen on.")
    args = parser.parse_args()
    start_server(host=args.host, port=args.port, should_print=args.print)
