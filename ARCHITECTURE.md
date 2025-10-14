# IEC Printer Architecture

## System Overview

```
┌─────────────────┐
│  Commodore 64   │
│                 │
│  PRINT#4,"..."  │
└────────┬────────┘
         │ IEC Bus (ATN, CLK, DATA)
         │
    ┌────▼────┐
    │ Pin 3   │ ATN
    │ Pin 4   │ CLK
    │ Pin 5   │ DATA
    └────┬────┘
         │
┌────────▼─────────────────────────────────┐
│     Arduino Uno R4 WiFi                  │
│                                          │
│  ┌────────────────────────────────────┐ │
│  │      IECBusHandler (library)       │ │
│  │  - Handles IEC protocol timing     │ │
│  │  - Manages ATN/CLK/DATA signals    │ │
│  │  - Routes commands to devices      │ │
│  └──────────────┬─────────────────────┘ │
│                 │                        │
│  ┌──────────────▼─────────────────────┐ │
│  │      IECPrinter (device 4)         │ │
│  │  - listen() - Receive print data   │ │
│  │  - write() - Buffer incoming bytes │ │
│  │  - talk() - Send status            │ │
│  │  - Detects job completion (EOI)    │ │
│  └──────────────┬─────────────────────┘ │
│                 │                        │
│  ┌──────────────▼─────────────────────┐ │
│  │      Main Loop (arduino.ino)       │ │
│  │  - Calls iecBus.task()             │ │
│  │  - Checks for completed jobs       │ │
│  │  - Sends data via WiFi             │ │
│  └──────────────┬─────────────────────┘ │
└─────────────────┼─────────────────────────┘
                  │ WiFi
                  │
         ┌────────▼────────┐
         │  Raspberry Pi   │
         │  python_server  │
         │  Port 65432     │
         └────────┬────────┘
                  │
         ┌────────▼────────┐
         │  PDF Converter  │
         │  lp command     │
         └────────┬────────┘
                  │
         ┌────────▼────────┐
         │  USB/WiFi       │
         │  Printer        │
         └─────────────────┘
```

## IEC Bus Communication Flow

### 1. C64 Sends Print Command
```
C64: OPEN 4,4,7        → Opens channel 7 on device 4 (graphics mode)
C64: PRINT#4,CHR$(8)   → Sends graphics data
C64: CLOSE 4           → Closes channel
```

### 2. IEC Bus Sequence
```
C64 pulls ATN low
  ↓
IECBusHandler detects ATN interrupt
  ↓
C64 sends: 0x24 (LISTEN device 4)
  ↓
IECBusHandler routes to IECPrinter
  ↓
IECPrinter.listen(7) called
  ↓
C64 sends: 0x67 (DATA channel 7)
  ↓
C64 releases ATN, sends data bytes
  ↓
IECPrinter.write(byte, eoi) called for each byte
  ↓
C64 sends: 0x3F (UNLISTEN)
  ↓
IECPrinter.unlisten() called
  ↓
After 2 second timeout OR EOI signal:
  Print job marked complete
```

### 3. Data Transfer to Raspberry Pi
```
Main loop detects completed job
  ↓
Retrieves buffered data from IECPrinter
  ↓
Connects to Raspberry Pi via WiFi
  ↓
Sends raw bytes over TCP socket
  ↓
Python server receives data
  ↓
Converts to PDF and prints
```

## Multi-Device Bus Handling

The IEC bus can have multiple devices:

```
         IEC Bus
           │
    ┌──────┼──────┬──────┐
    │      │      │      │
Device 4  Device 8  Device 9  ...
(Printer) (Drive)  (Drive)
```

### How IECDevice Library Handles This

1. **ATN Signal**: All devices monitor ATN line
2. **Address Byte**: C64 sends device number (e.g., 0x28 = LISTEN device 8)
3. **Device Check**: Each device checks if address matches
4. **Response**: Only addressed device responds
5. **Others Silent**: Non-addressed devices stay silent

### Why Old Code Failed

```
Old Code:
  ATN detected → ALWAYS pull DATA low
  Problem: Interfered with device 8 (disk drive)

New Code:
  ATN detected → Read address byte passively
  If address == 4 → Respond
  If address != 4 → Stay silent
```

## Print Job Detection Logic

```cpp
void IECPrinter::write(uint8_t data, bool eoi)
{
  m_buffer.push_back(data);
  m_lastDataTime = millis();
  
  if (eoi && m_buffer.size() > 100) {
    // EOI + substantial data = job complete
    m_printJobReady = true;
  }
}

int8_t IECPrinter::canWrite()
{
  if (!m_buffer.empty() && !m_printJobReady) {
    if (millis() - m_lastDataTime > 2000) {
      // 2 second timeout = job complete
      m_printJobReady = true;
    }
  }
  return 1;
}
```

### Why Two Methods?

1. **EOI Signal**: Fast detection for single-transmission prints
2. **Timeout**: Handles multi-part prints (banners) where EOI comes between parts

## Memory Usage

```
Program Storage: 68,692 bytes (26% of 262,144)
Dynamic Memory:   7,832 bytes (23% of 32,768)
Print Buffer:    ~16,384 bytes (reserved via std::vector)
```

The buffer can grow dynamically for large print jobs (certificates, banners).

## Error Handling

### IEC Protocol Errors
- Handled by IECBusHandler
- Automatic timeout and retry
- Error state cleared on next ATN

### WiFi Errors
- Connection failure logged to Serial
- Print data retained in buffer
- Can retry on next loop iteration

### Buffer Overflow
- std::vector grows dynamically
- Limited by available RAM (~24KB free)
- Large jobs (>20KB) may fail

## Performance

### IEC Bus Speed
- Standard: ~400 bytes/second
- Fast loaders: Not currently supported
- Sufficient for graphics printing

### WiFi Transfer
- Typically <1 second for full print job
- Depends on network conditions
- Non-blocking (doesn't hold IEC bus)

## Future Enhancements

Possible improvements:
1. Support JiffyDOS fast protocol
2. Add SD card buffering for very large jobs
3. Implement text mode (PETSCII conversion)
4. Support multiple printer emulations (MPS-803, 1525, etc.)
5. Add web interface for configuration
