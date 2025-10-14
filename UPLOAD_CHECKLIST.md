# Upload and Testing Checklist

## Pre-Upload

- [ ] Read `REFACTOR_SUMMARY.md` to understand changes
- [ ] Read `QUICK_START.md` for basic instructions
- [ ] Backup old Arduino code (if needed)
- [ ] Verify config.h has correct WiFi credentials
- [ ] Verify Raspberry Pi is running and accessible
- [ ] Verify python_server.py is running on Raspberry Pi

## Upload Process

### Option 1: Using Arduino CLI (Recommended)

```bash
cd /Users/chuck/Desktop/IEC-printer/arduino

# Install library and compile
./install_and_compile.sh

# Find your Arduino port
arduino-cli board list

# Upload (replace port as needed)
arduino-cli upload -p /dev/cu.usbmodem* --fqbn arduino:renesas_uno:unor4wifi .
```

### Option 2: Using Arduino IDE

1. Open Arduino IDE
2. File → Open → Select `arduino.ino`
3. Tools → Board → Arduino Renesas UNO R4 Boards → Arduino Uno R4 WiFi
4. Tools → Port → Select your Arduino port
5. Sketch → Upload

## Post-Upload Verification

### 1. Check Serial Monitor
- [ ] Open Serial Monitor (9600 baud)
- [ ] Should see: "IEC Printer Interface starting..."
- [ ] Should see: "IEC bus initialized on device 4"
- [ ] Should see: "Connected to WiFi"
- [ ] Should see: WiFi details (SSID, IP, signal)
- [ ] Should see: "IEC Printer initialized"

### 2. Test Disk Drive (Critical!)
```basic
LOAD "$",8
LIST
```
- [ ] Directory loads successfully
- [ ] No "DEVICE NOT PRESENT" errors
- [ ] Drive responds normally

### 3. Test Basic Print
```basic
OPEN 4,4
PRINT#4,"HELLO WORLD"
CLOSE 4
```

Expected Serial Output:
```
Print job complete: 11 bytes
=== SENDING PRINT JOB ===
11 bytes
Connected to server
Sent 11 bytes
```

- [ ] Serial shows "Print job complete"
- [ ] Data sent to Raspberry Pi
- [ ] No errors in Serial Monitor

## Application Testing

### Print Shop

#### Signs
- [ ] Load Print Shop
- [ ] Create a sign
- [ ] Select Print → MPS-803
- [ ] Print completes successfully
- [ ] PDF generated on Raspberry Pi

#### Letterheads
- [ ] Create a letterhead
- [ ] Print completes successfully
- [ ] PDF generated correctly

#### Banners (Multi-page)
- [ ] Create a banner (2-3 pages)
- [ ] Print completes successfully
- [ ] All pages captured in single PDF
- [ ] Serial shows multiple data transmissions

### Certificate Maker (The Big Test!)

- [ ] Load Certificate Maker
- [ ] Select a certificate template
- [ ] **CRITICAL**: Disk drive remains responsive
- [ ] Advance to "1 or 2 disks?" prompt
- [ ] Complete certificate design
- [ ] Print certificate
- [ ] Certificate prints successfully

## Troubleshooting

### Disk Drive Not Working

**Symptom**: "DEVICE NOT PRESENT" error

**Check**:
1. Unplug Arduino, test drive alone
2. Check pin connections (ATN=3, CLK=4, DATA=5)
3. Check Serial Monitor for IEC errors
4. Verify GND connection

**Fix**: If drive works alone but not with Arduino, there's still a bus conflict issue.

### Certificate Maker Hangs

**Symptom**: Hangs after selecting template

**Check**:
1. Serial Monitor for ATN activity
2. Does it show repeated ATN commands?
3. Any error messages?

**Fix**: This should be fixed by the refactor. If still happening, the IECDevice library may need configuration.

### Print Job Never Completes

**Symptom**: Data received but never sent to Pi

**Check**:
1. Serial Monitor - does it show "Print job complete"?
2. Wait 2 seconds after print command
3. Check if EOI signal is being sent

**Fix**: Increase `PRINT_TIMEOUT_MS` in IECPrinter.cpp

### WiFi Connection Failed

**Symptom**: "Connection to server failed!"

**Check**:
1. Verify Raspberry Pi is on network
2. Ping the server from your computer
3. Check python_server.py is running
4. Verify port 65432 is open

**Fix**: Update server address in config.h

## Success Criteria

✅ All checks passed when:
- Disk drive works with Arduino connected
- Basic print test works
- Print Shop signs/letterheads/banners work
- Certificate Maker loads and prints without hanging
- Serial Monitor shows clean operation
- PDFs generated on Raspberry Pi

## Rollback Plan

If the new code doesn't work:

1. The old code files were removed, but you can restore from git:
   ```bash
   git checkout HEAD~1 arduino/
   ```

2. Or manually recreate the old structure if needed

## Getting Help

If issues persist:

1. Check Serial Monitor output carefully
2. Review ARCHITECTURE.md for understanding
3. Check IECDevice library documentation
4. Verify hardware connections
5. Test components individually (drive, printer, WiFi)

## Notes

- The refactor uses a completely different IEC implementation
- Timing and behavior may differ slightly from old code
- This is expected and should be more reliable
- The IECDevice library is battle-tested with many C64 programs
