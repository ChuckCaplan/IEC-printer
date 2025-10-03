#include <WiFiS3.h>
#include <avr/pgmspace.h>

// This header is generated from your binary file (e.g., xxd -i my_image.bin > binary_data.h)
// #include "print_hh.h" // print shop sign
// #include "print_letterhead.h" // print shop letterhead
// #include "print_card.h" // print shop card
#include "print_cert.h" // certificate maker
// #include "print_banner.h" // print shop banner

// WiFi credentials - REPLACE with your network details
char ssid[] = "";
char pass[] = "";
int status = WL_IDLE_STATUS;

// Server details - REPLACE with your server's IP address
char server[] = "10.0.0.54";
int port = 65432;

WiFiClient client;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

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

  // attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection
    delay(10000);
  }

  Serial.println("Connected to WiFi");
  printWifiStatus();

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial
  if (client.connect(server, port)) {
    Serial.println("Connected to server");
    Serial.print("Sending binary data: ");
    Serial.print(print_dump_len);
    Serial.println(" bytes");
    
    // Send data in chunks to avoid watchdog timeout
    const size_t CHUNK_SIZE = 1024;
    size_t sent = 0;
    while (sent < print_dump_len) {
      size_t to_send = min(CHUNK_SIZE, print_dump_len - sent);
      client.write(print_dump + sent, to_send);
      sent += to_send;
      delay(10);  // Small delay to feed watchdog
    }

    // After sending, explicitly close the connection to signal the end of transmission.
    Serial.println("Data sent. Closing connection.");
    client.stop();
  }
}

void loop() {
  // if the server's disconnected, stop the client
  if (!client.connected()) {
    Serial.println();
    Serial.println("Disconnecting from server.");
    client.stop();

    // do nothing forevermore
    while (true);
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