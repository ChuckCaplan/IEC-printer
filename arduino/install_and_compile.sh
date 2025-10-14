#!/bin/bash

# Install IECDevice library
echo "Installing IECDevice library..."
ARDUINO_LIB_DIR="$HOME/Documents/Arduino/libraries"
mkdir -p "$ARDUINO_LIB_DIR"

# Copy library if not already there
if [ ! -d "$ARDUINO_LIB_DIR/IECDevice" ]; then
    cp -r ../reference/IECDevice "$ARDUINO_LIB_DIR/"
    echo "IECDevice library installed"
else
    echo "IECDevice library already installed"
fi

# Compile for Arduino Uno R4 WiFi
echo "Compiling for Arduino Uno R4 WiFi..."
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi .

echo "Done! To upload, run:"
echo "arduino-cli upload -p /dev/cu.usbmodem* --fqbn arduino:renesas_uno:unor4wifi ."
