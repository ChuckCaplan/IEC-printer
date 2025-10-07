# IEC-printer

Forked from https://github.com/smdprutser/IEC-printer, however at this point very little if any code is used from that project.

This is a work in progress and is not fully working. One complete, this project will allow a real Commodore 64 to print from the serial (IEC) port to a modern (USB) printer. When the C64 prints, data goes from the IEC (serial) port to an Arduino board that handles IEC protocol communication. The Arduino then passes the raw print data to a Python script running on a Raspberry PI (or Mac, PC, etc.). The Python script converts the raw printer data to BMP images, and prints to a USB printer via the lp command. This is 100% automated - just print from the C64 right to a USB printer.

# Project Folder Structure
 * /arduino - Arduino sketch to be uploaded to an Arduino Uno R4 Wifi to handle IEC protocol communication and send to a Python script running on a Raspberry PI (or other system) for printing.

* /scripts - A Python script that runs on a Raspberry Pi Zero W and listens for raw print data from the Arduino. It then converts the raw print data tp BMP images and prints via the lp command.

# Installation
1. [Arduino Installation](./arduino/README.md)
2. [Python Script Installation](./scripts/README.md)

# YouTube Demo
TBD

# License
This code is distributed under the GNU Public License
which can be found at http://www.gnu.org/licenses/gpl.txt

# DISCLAIMER:
The author is in no way responsible for any problems or damage caused by using this code. Use at your own risk.