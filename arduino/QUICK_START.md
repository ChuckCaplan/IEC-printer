# Quick Start Guide

## 1. Configure WiFi

Edit `config.h` with your network details:
```cpp
const char ssid[] = "YourWiFiSSID";
const char pass[] = "YourWiFiPassword";
const char* server = "raspberrypi.local";  // or IP address
const int port = 65432;
```

## 2. Compile and Upload

```bash
# One-time setup: Install library
./install_and_compile.sh

# Upload to Arduino
arduino-cli upload -p /dev/cu.usbmodem* --fqbn arduino:renesas_uno:unor4wifi .
```

Or use Arduino IDE:
1. Open `arduino.ino`
2. Select Board: "Arduino Uno R4 WiFi"
3. Select Port
4. Click Upload

## 3. Connect Hardware

```
C64 IEC Port -> Arduino
-----------------------
GND          -> GND
ATN          -> Pin 3
CLK          -> Pin 4
DATA         -> Pin 5
```

## 4. Test

### From C64:
```basic
OPEN 4,4
PRINT#4,"HELLO WORLD"
CLOSE 4
```

### Expected Serial Output:
```
IEC Printer Interface starting...
IEC bus initialized on device 4
Connected to WiFi
SSID: YourNetwork
IP: 192.168.1.xxx
IEC Printer initialized
Print job complete: 11 bytes
=== SENDING PRINT JOB ===
11 bytes
Connected to server
Sent 11 bytes
```

## 5. Print from Software

### Print Shop
- Load Print Shop
- Create a sign, letterhead, or banner
- Select "Print"
- Choose MPS-803 printer
- Print should work automatically

### Certificate Maker
- Load Certificate Maker
- Select a certificate template
- The disk drive should remain responsive (this was the bug!)
- Complete the certificate and print

## Troubleshooting

**"WiFi module failed!"**
- Check Arduino Uno R4 WiFi is properly connected
- Try resetting the Arduino

**"Connection to server failed!"**
- Verify Raspberry Pi is running and accessible
- Check `python_server.py` is running on the Pi
- Verify server address and port in config.h
- Try pinging the server from your computer

**Disk drive not working**
- Unplug Arduino, verify disk drive works alone
- Plug Arduino back in, check Serial Monitor for errors
- Verify pin connections (especially ATN on pin 3)

**Print job never completes**
- Check Serial Monitor for "Print job complete" message
- Data may be buffering - wait 2 seconds after print command
- For very large jobs, increase PRINT_TIMEOUT_MS in IECPrinter.cpp

## Key Improvements Over Old Code

✅ Disk drive works reliably with Arduino connected
✅ Certificate Maker no longer hangs
✅ Proper IEC protocol timing
✅ Better handling of multi-part prints (banners)
✅ Cleaner, more maintainable code
✅ Based on proven IECDevice library
