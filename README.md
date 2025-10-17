# IEC-printer

Originally forked from https://github.com/smdprutser/IEC-printer, but now using https://github.com/dhansel/IECDevice for IEC protocol communication.

This project allows a real Commodore 64 to print from the serial (IEC) port to a modern (USB or wifi) printer. When the C64 prints, data goes from the IEC (serial) port to an Arduino UNO R4 Wifi that handles IEC protocol communication. The Arduino then passes the raw print data to a Python script running on a Raspberry PI (or Mac, PC, etc.). The Python script converts the raw printer data to PDF, and prints to a printer via the lp command. This is 100% automated - just print from the C64 right to a USB / wifi printer.

Working:
 * The Print Shop: Signs, greeting cards, letterheads, and multi-page banners
 * Certificate Maker
 * Presumably other software that prints in graphics mode, though that has not been tested.

Not Working / Issues:
 * Text-based printing like word processors
 * LOAD"*",8,1 sometimes needs to be retried once (DEVICE NOT FOUND)

# Project Folder Structure
 * /arduino - Arduino sketch to be uploaded to an Arduino UNO R4 Wifi to handle IEC protocol communication and send to a Python script running on a Raspberry PI (or other system) for printing.

* /scripts - A Python script that runs on a system like a Raspberry Pi Zero W and listens for raw print data from the Arduino. It then converts the raw print data to PDF and prints via the lp command.

# Installation
1. [Arduino Installation](./arduino/README.md)
2. [Python Script Installation](./scripts/README.md)

# Example
 This PDF was generated from The Print Shop on a real C64, passed to an Arduino UNO R4 Wifi, converted to PDF on a Raspberry Pi Zero W, and printed to a Brother wifi printer.\
 [![Example](examples/1760386301.316215.png)](examples/1760386301.316215.pdf)\
 Other Pics Outlining Workflow:
 * [C64 Setup](examples/1setup.jpeg)
 * [Print Shop Main Menu](examples/2ps_menu.jpeg)
 * [Print Shop Printing Screen](examples/3ps_printing.jpeg)
 * [Arduino UNO R4 Wifi (unplugged in this pic)](examples/4arduino.jpeg)
 * [Raspberry Pi Zero W](examples/5raspberrypi.jpeg)
 * [Printer](examples/6printer.jpeg)
 * [Printout](examples/7printout.jpeg)




# Printer Support
This project currently supports the [Commodore MPS-803](https://www.historybit.it/wp-content/uploads/2020/06/Manual_MPS-803.pdf) printer, but could be extended to support other printers with some additional work. It also only supports graphics mode. Text mode, including PETSCII characters and control codes for underlining, bold, etc. is not currently supported. This was tested with Broderbund The Print Shop and Springboard Certificate Maker printing to what the C64 thought was an MPS-803 printer.

# License
This code is distributed under the GNU Public License which can be found at http://www.gnu.org/licenses/gpl.txt

# DISCLAIMER:
The author is in no way responsible for any problems or damage caused by using this code. Use at your own risk.