#include <WiFiS3.h>
#include <avr/pgmspace.h>
#include "config.h"
#include <vector>
#include "globals.h"
#include "iec_driver.h"
#include "interface.h"

// ---- Pin & device configuration ----
const byte IEC_ATN_PIN = 3;
const byte IEC_CLK_PIN = 4;
const byte IEC_DATA_PIN = 5;
const byte CBM_DEVICE_NUMBER = 4; // Printer device

// ---- Globals ----
IEC iec(CBM_DEVICE_NUMBER);
Interface interface(iec);
std::vector<byte> printDataBuffer;
int status = WL_IDLE_STATUS;
WiFiClient client;

// ---- Forward declarations ----
void connectToWiFi();
void printWifiStatus();
void sendDataToServer();

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }

  Serial.println("IEC Printer Interface starting...");
  printDataBuffer.reserve(16384);

  // WiFi module present?
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true) { delay(100); }
  }

  // Initialize IEC pins
  iec.setPins(IEC_ATN_PIN, IEC_CLK_PIN, IEC_DATA_PIN);
  iec.init();
  iec.setDeviceNumber(CBM_DEVICE_NUMBER);
  Serial.print("Device number: "); Serial.println(CBM_DEVICE_NUMBER);

  // Connect WiFi
  connectToWiFi();
}

void loop() {

  // Handle IEC communication
  IEC::ATNCheck atnResult = interface.handler();

  // Debug
  if (atnResult == IEC::ATN_ERROR) {
    Serial.println("ATN ERROR!");
  } else if (atnResult != IEC::ATN_IDLE) {
    Serial.print("ATN Result: ");
    Serial.println((int)atnResult);
  }

  // Ship a finished print job to the server
  if (interface.isPrintJobActive()) {
    Serial.println("\n=== PRINT JOB RECEIVED ===");
    Serial.print(printDataBuffer.size());
    Serial.println(" bytes of data captured.");
    if (!printDataBuffer.empty()) {
      sendDataToServer();
    }
  }

  delay(1);
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
    client.write(printDataBuffer.data(), printDataBuffer.size());
    Serial.print("Sent "); Serial.print(printDataBuffer.size()); Serial.println(" bytes to server");
    client.stop();
    printDataBuffer.clear();
    interface.printJobHandled();
  } else {
    Serial.println("Connection to server failed!");
  }
}

void printWifiStatus() {
  Serial.print("SSID: "); Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: "); Serial.println(ip);
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):"); Serial.print(rssi); Serial.println(" dBm");
}
