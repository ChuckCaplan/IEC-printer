# Old Scripts

My original plan was to take raw printer data from the C64 (via the Arduino), package it as a disk image, and print in the [Vice](https://vice-emu.sourceforge.io/) C64 emulator to print as a BMP, and then print to a USB printer. This actually works really well for all type of MPS-803 (and other printer) jobs. However, converting a simple sign from The Print Shop to BMP with this script takes ~30 seconds to run on my Mac, and multiple minutes on a Raspberry Pi Zero W. Therefore I decided to write the entire conversion process in Python., which runs in seconds, even though it doesn't have the same level of MPS-803 support. I am leaving this here in case anyone finds this script useful.

# License

This code is distributed under the GNU Public License
which can be found at http://www.gnu.org/licenses/gpl.txt

# DISCLAIMER:
The author is in no way responsible for any problems or damage caused by using this code. Use at your own risk.