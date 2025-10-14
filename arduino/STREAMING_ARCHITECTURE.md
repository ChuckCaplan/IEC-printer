# Streaming Architecture for Large Print Jobs

## Problem

Print jobs from Certificate Maker and Print Shop banners can be **45-58KB**, but the Arduino Uno R4 WiFi only has ~25KB of free RAM. We cannot buffer entire jobs in memory.

## Solution: Stream to WiFi

Instead of buffering data in RAM, we **stream it directly to the Raspberry Pi** as it arrives from the C64.

## How It Works

```
C64 sends byte → Arduino receives → Chunk buffer (512 bytes)
                                           ↓
                                    Chunk full?
                                           ↓
                                    Send to WiFi immediately
                                           ↓
                                    Clear chunk buffer
                                           ↓
                                    Ready for more data
```

### Key Components

1. **Chunk Buffer**: 512 bytes (small, fits in RAM)
2. **WiFi Connection**: Opened at start of print job
3. **Streaming**: Data sent in 512-byte chunks as received
4. **Timeout**: 3 seconds after last data = job complete

### Memory Usage

```
Old (buffering):
- Buffer: 58KB (doesn't fit!)
- Result: Out of memory crash

New (streaming):
- Chunk: 512 bytes
- WiFi client: ~2KB
- Total: ~3KB
- Result: Works for any size job!
```

## Print Job Flow

### 1. Job Start
```
C64: OPEN 4,4,7
  ↓
Arduino: listen(7) called
  ↓
Arduino: startPrintJob()
  ↓
Connect to Raspberry Pi
  ↓
Ready to stream
```

### 2. Data Transfer
```
C64 sends byte
  ↓
Arduino: write(byte) called
  ↓
Add to chunk[512]
  ↓
Chunk full?
  ↓ YES
Send chunk to WiFi
Clear chunk
  ↓ NO
Wait for more data
```

### 3. Job End

**Method 1: EOI Signal**
```
C64 sends last byte with EOI
  ↓
Arduino: write(byte, eoi=true)
  ↓
Flush remaining chunk
  ↓
Close WiFi connection
```

**Method 2: Timeout**
```
3 seconds pass with no data
  ↓
Arduino: task() detects timeout
  ↓
Flush remaining chunk
  ↓
Close WiFi connection
```

## Multi-Part Prints (Banners)

Print Shop banners send data in multiple parts:

```
Part 1: OPEN 4,4,7 → data → UNLISTEN
  ↓ (streaming to WiFi)
Part 2: OPEN 4,4,7 → data → UNLISTEN
  ↓ (still streaming to same connection)
Part 3: OPEN 4,4,7 → data → CLOSE
  ↓ (streaming continues)
3 second timeout
  ↓
Job complete, close connection
```

The WiFi connection **stays open** across multiple OPEN/UNLISTEN cycles, allowing all parts to stream to the same job.

## Advantages

1. **Handles Any Size**: No RAM limit on print job size
2. **Fast**: Data streams immediately, no waiting to buffer
3. **Reliable**: Less memory pressure = more stable
4. **Simple**: No complex buffer management

## Code Structure

### IECPrinter.h
```cpp
uint8_t m_chunk[512];      // Small chunk buffer
uint16_t m_chunkPos;       // Current position in chunk
WiFiClient m_client;       // WiFi connection
bool m_inPrintJob;         // Are we streaming?
```

### Key Functions

**startPrintJob()**
- Opens WiFi connection to Raspberry Pi
- Initializes chunk buffer
- Sets m_inPrintJob = true

**write(byte, eoi)**
- Adds byte to chunk
- Flushes chunk if full (512 bytes)
- If EOI, flushes and ends job

**flushChunk()**
- Sends chunk to WiFi
- Resets chunk position

**endPrintJob()**
- Flushes remaining data
- Closes WiFi connection
- Resets state

**task()**
- Called from main loop
- Checks for 3-second timeout
- Ends job if timeout detected

## Testing with Example Data

### Banner (58,610 bytes)
```
Chunks sent: 58610 / 512 = 115 chunks
Memory used: 512 bytes (chunk) + 2KB (WiFi) = ~3KB
Result: ✅ Fits in RAM
```

### Certificate (45,430 bytes)
```
Chunks sent: 45430 / 512 = 89 chunks
Memory used: 512 bytes (chunk) + 2KB (WiFi) = ~3KB
Result: ✅ Fits in RAM
```

### Letterhead (23KB)
```
Chunks sent: 23000 / 512 = 45 chunks
Memory used: 512 bytes (chunk) + 2KB (WiFi) = ~3KB
Result: ✅ Fits in RAM
```

## IEC Bus Compatibility

The streaming approach does NOT interfere with the disk drive because:

1. **IECDevice library** handles all IEC protocol correctly
2. Arduino only responds when addressed as device 4
3. Data reception is fast (no blocking on WiFi)
4. Chunk sends happen between IEC bytes (plenty of time)

## Performance

### IEC Bus Speed
- ~400 bytes/second from C64
- Plenty of time to send 512-byte chunks

### WiFi Speed
- 512 bytes takes ~10ms to send
- IEC byte arrives every ~2.5ms
- No bottleneck

### Timeout Tuning
- 3 seconds is safe for multi-part prints
- Adjust `PRINT_TIMEOUT_MS` if needed
- Too short = premature job end
- Too long = delay before printing

## Error Handling

### WiFi Connection Lost
```cpp
if (!m_client.connected()) {
  Serial.println("Connection lost!");
  endPrintJob();
  return 0; // Signal error to IEC
}
```

### Server Not Available
```cpp
if (!m_client.connect(server, port)) {
  Serial.println("Connection failed!");
  // Don't start job
}
```

## Future Enhancements

1. **Retry Logic**: Reconnect if WiFi drops
2. **Buffering Option**: Small buffer for network hiccups
3. **Compression**: Compress data before sending
4. **Multiple Jobs**: Queue if new job starts during print
