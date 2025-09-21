# IEC-printer

Forked from https://github.com/smdprutser/IEC-printer.

This is a work in progress and is not fully working. One complete, this project will allow a real Commodore 64 to print from the serial (IEC) port to a modern (USB) printer. The C64 will print and data will go from the serial port to an Arduino board that will handle IEC protocol communication. The Arduino will then pass the raw print data to a Raspberry PI (or Mac, PC, etc.). From there it will be sent to the Vice C64 emulator and output BMP images, which will then print to a USB printer via the lpr command. This can add between 20 seconds and several minutes to the normal C64 print time depending on how many pages are being printed. Once working this should be 100% automated - just print from the C64 right to a USB printer.

# Project Folder Structure
 * /arduino - Arduino sketch to be uploaded to an Arduino Uno to handle IEC protocol communication and send to a Raspberry PI for printing.
* /scripts - Shell scripts that will run on the Raspberry pi to listen on the USB serial port for raw print data from the Arduino, call Vice to generate BMP images from the data, and print via the lpr command.

# License
This code is distributed under the GNU Public License
which can be found at http://www.gnu.org/licenses/gpl.txt

# DISCLAIMER:
The author is in no way responsible for any problems or damage caused by using this code. Use at your own risk.