# IEC-printer

Forked from https://github.com/smdprutser/IEC-printer.

This project allows a real Commodore 64 to print from the serial (IEC) port to a modern (USB or wifi) printer. When the C64 prints, data goes from the IEC (serial) port to an Arduino board that handles IEC protocol communication. The Arduino then passes the raw print data to a Python script running on a Raspberry PI (or Mac, PC, etc.). The Python script converts the raw printer data to PDF, and prints to a printer via the lp command. This is 100% automated - just print from the C64 right to a USB / wifi printer.

This is a work in progress and is currently working as a POC for signs, banners, and letterheads in [The Print Shop](https://en.wikipedia.org/wiki/The_Print_Shop).

Working:
 *  Print Shop signs, letterheads, and multi-page banners

Not Working:
 * Print Shop Cards - Prints one page per card quarter
 * Certificate Maker - Disk error due to IEC communication
 * Text-based printing like word processors

To Be Tested:
 * Newsroom

# Project Folder Structure
 * /arduino - Arduino sketch to be uploaded to an Arduino Uno R4 Wifi to handle IEC protocol communication and send to a Python script running on a Raspberry PI (or other system) for printing.

* /scripts - A Python script that runs on a system like a Raspberry Pi Zero W and listens for raw print data from the Arduino. It then converts the raw print data to PDF and prints via the lp command.

# Installation
1. [Arduino Installation](./arduino/README.md)
2. [Python Script Installation](./scripts/README.md)

# Example
 This PDF was generated from The Print Shop on a real C64, passed through an Arduino Uno R4 Wifi, converted to PDF on a Raspberry Pi Zero W, and printed to a Brother wifi printer.\
 [![Example](examples/1760386301.316215.png)](examples/1760386301.316215.pdf)

# Printer Support
This project currently supports the MPS-803 printer, but could be extended to support other printers with some additional work. It also only currently supports graphics mode. Text mode, including PETSCII characters and control codes for underlining, bold, etc. is not currently supported. This was tested with Broderbund The Print Shop and Springboard Certificate Maker printing to what the C64 thought was an [MPS-803](https://www.historybit.it/wp-content/uploads/2020/06/Manual_MPS-803.pdf) printer.

# License
This code is distributed under the GNU Public License which can be found at http://www.gnu.org/licenses/gpl.txt

# DISCLAIMER:
The author is in no way responsible for any problems or damage caused by using this code. Use at your own risk.