# IEC-printer - Arduino Installation

This Arduino sketch sends raw print data from a C64 to a Raspberry PI for printing to a USB or wifi printer, handling all IEC protocol handshaking and communication using the mature IECDevice library.

**Note**: This code has been refactored to use the IECDevice library for reliable IEC bus communication. See REFACTOR_NOTES.md for details. 

Make sure your project has a file named config.h with the following content:\
// WiFi credentials - REPLACE with your network details\
char ssid[] = "your_SSID";\
char pass[] = "your_PASSWORD";\
// Server details - REPLACE with your server's IP address or hostname. This is where the Python script to receive the print data will be running.\
char server[] = "raspberrypi.local";\
int port = 65432;

# Hardware
 * Arduino Uno R4 Wifi
 * IEC cable to connect the C64 to the Arduino via the following pins:
    - IEC GND to Arduino GND
    - IEC ATN to Arduino digital pin 3
    - IEC CLK to Arduino digital pin 4
    - IEC DATA to Arduino digital pin 5

# Info
This implementation uses the IECDevice library by David Hansel (https://github.com/dhansel/IECDevice) which provides robust, well-tested IEC protocol handling. The library properly handles multi-device scenarios, ensuring the printer coexists peacefully with disk drives and other IEC devices.

# License
This code is distributed under the GNU Public License
which can be found at http://www.gnu.org/licenses/gpl.txt

# DISCLAIMER:
The author is in no way responsible for any problems or damage caused by using this code. Use at your own risk.