/*
Purpose:
 This sketch collects data from an FDC1004 capacitance sensor and sends it
 to a Flask server. The Flask server will then update the corresponding
 Firebase realtime database.

Notes:
 1. This example is written for a network using WPA encryption.
 2. Circuit: Arduino Nano IoT, FDC1004 capacitance sensor breakout board.
 3. Update the network SSID, password and server IP address as described in the comments.
 4. The GET request now sends the sensor value as "capacitance" instead of "distance".
*/

// Library Inclusions
#include <SPI.h>              // For wireless communications
#include <WiFiNINA.h>         // Used to connect Nano IoT to network
#include <ArduinoJson.h>      // Used for HTTP Request (if needed for JSON processing)
#include "arduino_secrets.h"  // Contains your sensitive WiFi credentials

// Include libraries for the capacitance sensor
#include <Wire.h>
#include <Protocentral_FDC1004.h>

// WiFi network credentials and server configuration
char ssid[] = SECRET_SSID;    // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password
int keyIndex = 0;             // your network key index number (needed only for WEP)
int status = WL_IDLE_STATUS;

// Initialize the WiFi client library
WiFiClient client;

// Server address:
// Update this IP with the computer running your Flask server (using commas here)
IPAddress server(172,20,10,5);

// Timing variables for HTTP posting
unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 10L * 50L; // around 1 second between requests

// --- Capacitance Sensor Globals ---
// Define constants for the FDC1004 sensor reading
#define UPPER_BOUND  0x4000                 // max readout threshold
#define LOWER_BOUND  (-1 * UPPER_BOUND)
#define CHANNEL 0                          // channel to be read
#define MEASURMENT 0                       // measurement channel index

int capdac = 0;                           // adjustment factor for the sensor
FDC1004 FDC;                              // create an instance of the FDC1004 sensor

// Global variable to hold the capacitance sensor reading (in picoFarads)
float capacitanceValue = 0.0;

void setup(){
  Serial.begin(9600); // Start serial monitor
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  // Check if firmware is outdated:
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // Attempt to connect to the WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(1000);
  }
  printWifiStatus(); // Connected; print status

  // Initialize the I2C bus and the FDC1004 sensor
  Wire.begin();
}

void loop(){
  // Check for incoming data from the server and print if available
  String response = "";
  while (client.available()) {
    char c = client.read();
    response += c;
  }
  if (response != "") {
    Serial.println(response);
  }
  
  // Send a new HTTP request every postingInterval milliseconds
  if (millis() - lastConnectionTime > postingInterval) {
    httpRequest();
  }
}

// Makes an HTTP GET request to the Flask server with the sensor value
void httpRequest() {
  // Close any previous connection to free the socket
  client.stop();

  // Read the capacitance sensor value
  readCapacitance();
  
  // Attempt to connect to the server on the specified port (update if needed)
  if (client.connect(server, 5000)) {
    Serial.println("connecting...");
    // Prepare the HTTP GET request; note that the parameter is now "capacitance"
    String request = "GET /test?capacitance=" + String(capacitanceValue) + " HTTP/1.1";
    client.println(request);
    
    // Set the Host header (update the IP as needed using commas in server() above and periods here)
    client.println("Host: 192.0.0.2");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();

    // Record the time of this connection
    lastConnectionTime = millis();
  } else {
    Serial.println("connection failed");
  }
}

// Prints the current WiFi connection status to the serial monitor
void printWifiStatus(){
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// Reads the capacitance sensor value from the FDC1004 and stores it in 'capacitanceValue'
void readCapacitance(){
  // Configure and trigger a single measurement on the defined channel
  FDC.configureMeasurementSingle(MEASURMENT, CHANNEL, capdac);
  FDC.triggerSingleMeasurement(MEASURMENT, FDC1004_100HZ);

  // Wait for the measurement to complete
  delay(15);
  
  uint16_t value[2];
  if (!FDC.readMeasurement(MEASURMENT, value)) {
    int16_t msb = (int16_t) value[0];
    // Calculate raw capacitance in femtofarads, then convert to picoFarads (pF)
    int32_t capCalc = ((int32_t)457) * ((int32_t)msb);
    capCalc /= 1000;
    capCalc += ((int32_t)3028) * ((int32_t)capdac);
    capacitanceValue = capCalc / 1000.0;  // Now in picoFarads
    Serial.print(capacitanceValue, 4);
    Serial.println(" pF");
    
    // Adjust capdac based on reading thresholds
    if (msb > UPPER_BOUND) {
      if (capdac < FDC1004_CAPDAC_MAX)
        capdac++;
    } else if (msb < LOWER_BOUND) {
      if (capdac > 0)
        capdac--;
    }
  }
  delay(50);
}
