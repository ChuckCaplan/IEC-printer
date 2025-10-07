# IEC-printer - Script Installation

This Python script accepts raw MPS-803 printer data from an Arduino, converts to BMP images, and optionally prints to the default installed printer using the lp command. When all connected, MPS-803 print jobs from the C64 will automatically print to a modern (USB) printer.

# Installation

These instructions assume you are using a Raspberry Pi Zero W and printing to a wireless (wi-fi) printer. Installing on other types of systems will be very similar but will need some modifications.

1. TBD

# Testing
To test this script on its own, you can pass in a raw printer data file from Vice using nc (or netcat on some systems):

nc localhost 65432 < print.dump

# License

This code is distributed under the GNU Public License
which can be found at http://www.gnu.org/licenses/gpl.txt

# DISCLAIMER:
The author is in no way responsible for any problems or damage caused by using this code. Use at your own risk.