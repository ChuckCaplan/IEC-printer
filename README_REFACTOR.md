# Arduino IEC Printer - Complete Refactor

## What Was Done

Complete rewrite of Arduino IEC communication using the **IECDevice library** with **streaming architecture** for large print jobs.

## Critical Design Decisions

### 1. Streaming Instead of Buffering
**Problem**: Print jobs are 45-58KB, Arduino only has 25KB free RAM

**Solution**: Stream data directly to WiFi in 512-byte chunks
- Memory usage: ~4KB (vs 58KB needed for buffering)
- Handles unlimited job sizes
- No out-of-memory crashes

### 2. IECDevice Library
**Problem**: Custom IEC driver had bus conflicts with disk drive

**Solution**: Use proven IECDevice library
- Proper multi-device support
- Only responds when addressed as device 4
- Disk drive (device 8) works normally

## Files

### New
- `IECPrinter.cpp/h` - Streaming printer device
- `arduino.ino` - Refactored main sketch
- `STREAMING_ARCHITECTURE.md` - Technical details
- `FINAL_VERIFICATION.md` - Complete verification

### Removed
- `iec_driver.cpp/h` - Buggy custom IEC code
- `interface.cpp/h` - No longer needed
- `globals.h` - Simplified

## Compilation

```bash
cd arduino
./install_and_compile.sh
arduino-cli upload -p /dev/cu.usbmodem* --fqbn arduino:renesas_uno:unor4wifi .
```

Memory: 26% flash, 25% RAM ✅

## Testing

1. Disk drive works with Arduino connected ✅
2. Print Shop signs/letterheads/banners ✅
3. Certificate Maker (no disk hang) ✅
4. Large jobs (45-58KB) ✅

See `UPLOAD_CHECKLIST.md` for detailed testing steps.
