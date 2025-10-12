#include <WiFiS3.h>
#include <avr/pgmspace.h>
#include "config.h"
#include <vector>
#include "globals.h"
#include "iec_driver.h"
#include "interface.h"

/*
Make sure your project has a file named config.h with the following content:
// WiFi credentials - REPLACE with your network details
char ssid[] = "your_SSID";
char pass[] = "your_PASSWORD";
// Server details - REPLACE with your server's IP address or hostname. This is where the Python script to receive the print data will be running.
char server[] = "raspberrypi.local";
int port = 65432;
*/

// IEC Pin configuration
const byte IEC_ATN_PIN = 3;
const byte IEC_CLK_PIN = 4;
const byte IEC_DATA_PIN = 5;
const byte CBM_DEVICE_NUMBER = 4; // Standard printer device number

// IEC communication objects
IEC iec(CBM_DEVICE_NUMBER);
Interface interface(iec);

// Buffer to hold incoming print data from C64
std::vector<byte> printDataBuffer;

// initial wifi status
int status = WL_IDLE_STATUS;
WiFiClient client;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("IEC Printer Interface starting...");

  // Reserve some space for the print buffer to avoid reallocations during reception
  printDataBuffer.reserve(8192);

  // check for the WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // Initialize IEC bus communication
  iec.setPins(IEC_ATN_PIN, IEC_CLK_PIN, IEC_DATA_PIN);
  iec.init();
  Serial.println("IEC listener initialized.");
  Serial.print("Listening as device #");
  Serial.println(CBM_DEVICE_NUMBER);

  // attempt to connect to WiFi network
  connectToWiFi();
}

void loop() {
  // Debug: Show ATN line state occasionally
  static unsigned long lastDebug = 0;
  if(millis() - lastDebug > 5000) {
    Serial.print(".");
    lastDebug = millis();
  }
  
  // Handle IEC communication with the C64
  IEC::ATNCheck atnResult = interface.handler();
  
  // Debug output
  if(atnResult == IEC::ATN_ERROR) {
    Serial.println("ATN ERROR!");
  } else if(atnResult != IEC::ATN_IDLE) {
    Serial.print("ATN Result: ");
    Serial.println(atnResult);
  }

  // Check if a print job has finished and is ready to be sent
  if (interface.isPrintJobActive()) {
    Serial.println("\n=== PRINT JOB RECEIVED ===");
    Serial.print(printDataBuffer.size());
    Serial.println(" bytes of data captured.");

    if (printDataBuffer.size() > 0) {
      sendDataToServer();
    }

    // Clear buffer and reset state for the next job
    printDataBuffer.clear();
    interface.printJobHandled();
    Serial.println("=== READY FOR NEXT JOB ===\n");
  }
}

void connectToWiFi() {
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
  Serial.println("Connected to WiFi");
  printWifiStatus();
}

void sendDataToServer() {
  Serial.println("Connecting to server to send data...");
  if (client.connect(server, port)) {
    Serial.println("Connected to server");
    
    const size_t CHUNK_SIZE = 1024;
    client.write(printDataBuffer.data(), printDataBuffer.size());

    Serial.println("Data sent. Closing connection.");
    client.stop();
  } else {
    Serial.println("Connection to server failed!");
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}