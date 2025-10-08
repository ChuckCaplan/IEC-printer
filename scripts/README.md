# IEC-printer - Script Installation

This Python script accepts raw MPS-803 printer data from an Arduino, converts to PDF, and optionally prints to the default installed printer using the lp command. When all connected, MPS-803 print jobs from the C64 will automatically print to a modern (USB) printer.

# Installation

These instructions assume you are using a Raspberry Pi Zero W and printing to a wireless (wi-fi) printer. Installing on other types of systems will be very similar but will need some modifications.

## 1. Install Raspberry Pi OS

a. Raspberry Pi Imager
- Choose Device --> Raspberry Pi Zero W (I am using the Zero W and not the Zero 2 W)
- Choose OS --> Raspberry Pi OS (Other) --> Raspberry Pi OS (Legacy 32-bit) Lite (This installs Bookworm, which is an older, more stable, faster OS for the Zero W)
- Choose Storage --> Your SD Card
- Next
- Edit Settings
- Hostname - raspberrypi.local
- Set username and password
- Configure wireless LAN with wi-fi info
- Set locale settings
- Services/Enable SSH - Use password authentication
- Save
- Would you like to apply? Yes
- Do you want to continue? Yes
- Enter system PW if needed

b. Plug in, wait for boot, ssh in:
- ssh username@raspberrypi.local

c. Install updates:
- sudo apt update
- sudo apt full-upgrade -y
- sudo reboot -h now
- Then login again

d. Confirm Python is installed. If not, install it on your own.
- python3 --version

## 2. Install Required Packages
- sudo apt install -y python3-numpy python3-pil git

git --version

## 3. Install Printer Support
a. Install CUPS
- sudo apt install cups avahi-utils -y
- sudo systemctl enable --now cups
- sudo systemctl enable --now avahi-daemon

b. Display the printers that CUPS sees. Your printer will hopefully be in this list. If not you will need to troubleshoot. Try rebooting (command above) to make sure  Avahi and CUPS-browsed finish their startup handshake and can see your wi-fi printer. Copy the ipp:// address to be used later.
- sudo lpinfo -v

c. Install your wi-fi printer as the default
- sudo lpadmin -p Brother -E -v "ipp://Brother%20HL-L8360CDW%20series._ipp._tcp.local/" -m everywhere
- sudo lpadmin -d Brother
- Confirm the newly installed printer is the default:\
lpstat -p -d\
It should say:\
system default destination: Brother

d. Print a test page if needed (upload with sftp):
- lp test.pdf

## 4. Install the Python Script
a. Clone the IEC-Printer repo.
- cd ~    
- git clone https://github.com/ChuckCaplan/IEC-printer.git

## 5. Install the Service

a. Install on startup
- Edit iec-printer.service and change the 3 instances of your_username to your username (replace chuck with your username):\
sed -i 's/your_username/chuck/g' ~/IEC-printer/scripts/iec-printer.service
- sudo cp ~/IEC-printer/scripts/iec-printer.service /etc/systemd/system/
- sudo systemctl daemon-reload
- sudo systemctl enable iec-printer.service
- sudo systemctl start iec-printer.service

b. To check the status (if needed):
- sudo systemctl status iec-printer.service
- journalctl -u iec-printer.service -f

c. To stop the service (if needed):
- sudo systemctl stop iec-printer.service

d. To restart the service (if needed):
- sudo systemctl restart iec-printer.service

## 6. Testing

a. Run the Python server and test if works. If you installed the service then this should already be running. Use the -p command to automatically print the converted PDF to your default printer. Leave it out if you only want to save the image to disk.
- cd ~/IEC-printer/scripts
- python3 python_server.py -p

b. To test this script on its own, you can pass in a raw printer data file from Vice using nc (or netcat on some systems):

- nc localhost 65432 < print.dump
- Then look in ~/IEC-printer/output for the generated PDF

c. If needed, install an X-Windows PDF viewer
- sudo apt install -y mupdf
- Then on Mac, run X Quartz:
- ssh -X user@raspberrypi.local
- mupdf ~/IEC-printer/output/mypdf.pdf

# License

This code is distributed under the GNU Public License
which can be found at http://www.gnu.org/licenses/gpl.txt

# DISCLAIMER:
The author is in no way responsible for any problems or damage caused by using this code. Use at your own risk.