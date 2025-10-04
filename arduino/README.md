# IEC-printer

Forked from https://github.com/smdprutser/IEC-printer.

Once working this Arduino sketch will send raw print data from a C64 to a Raspberry PI for printing to a USB printer. This is currently NOT WORKING for this use case.

Make sure your project has a file named config.h with the following content:
// WiFi credentials - REPLACE with your network details
char ssid[] = "your_SSID";
char pass[] = "your_PASSWORD";
// Server details - REPLACE with your server's IP address. This is where the Python script to receive the print data will be running.
char server[] = "your_server_ip";
int port = your_server_port;

# Info
Most of the IEC routines borowed from https://github.com/Larswad/uno2iec . Many thanks for solving the gory details of the IEC and kept me focussing at the printercode.

# License
This code is distributed under the GNU Public License
which can be found at http://www.gnu.org/licenses/gpl.txt

# DISCLAIMER:
The author is in no way responsible for any problems or damage caused by using this code. Use at your own risk.



