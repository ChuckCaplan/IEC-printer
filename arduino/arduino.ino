#include <WiFiS3.h>
#include "config.h"
#include "IECPrinter.h"
#include "IECBusHandler.h"

// Pin configuration
const byte IEC_ATN_PIN = 3;
const byte IEC_CLK_PIN = 4;
const byte IEC_DATA_PIN = 5;

// IEC bus and printer device
IECBusHandler iecBus(IEC_ATN_PIN, IEC_CLK_PIN, IEC_DATA_PIN);
IECPrinter printer;

// WiFi
int wifiStatus = WL_IDLE_STATUS;

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }

  Serial.println("IEC Printer Interface starting...");

  // Check WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module failed!");
    while (true) { delay(100); }
  }

  // Connect to WiFi
  connectToWiFi();
  
  // Configure printer with server info
  printer.setServerInfo(server, port);
  
  // Attach printer to IEC bus
  iecBus.attachDevice(&printer);
  iecBus.begin();
  
  Serial.println("IEC bus initialized on device 4");
  Serial.println("Ready to print!");
}

void loop() {
  // Handle IEC bus communication
  iecBus.task();
  
  // Handle printer timeout logic
  printer.task();

  delay(1);
}

void connectToWiFi() {
  while (wifiStatus != WL_CONNECTED) {
    Serial.print("Connecting to: ");
    Serial.println(ssid);
    wifiStatus = WiFi.begin(ssid, pass);
    delay(10000);
  }
  Serial.println("Connected to WiFi");
  printWifiStatus();
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}
