/*
  Batch‐uploading capacitance readings from an FDC1004 to a Flask server

  This sketch buffers N readings from the FDC1004 sensor, then
  sends them all at once as a JSON array via HTTP POST to your Flask
  endpoint at /test. You'll need to define your WiFi creds in
  arduino_secrets.h, and update the server IP below.
*/

// ——— Library Inclusions ————————————————————————————————————————————————
#include <SPI.h>
#include <WiFiNINA.h>            // Nano IOT WiFi
#include <ArduinoJson.h>         // (Optional – you can build JSON manually)
#include "arduino_secrets.h"     // SECRET_SSID, SECRET_PASS

#include <Wire.h>
#include <Protocentral_FDC1004.h>  // Adafruit/FDC1004 capacitance sensor

// ——— WiFi and Server Config ————————————————————————————————————————
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int  status = WL_IDLE_STATUS;
WiFiClient client;
// Update this to the IP of your Flask server:
IPAddress server(172, 20, 10, 5);

// ——— Buffer Settings —————————————————————————————————————————————
#define BATCH_SIZE  5               // send every 5 readings
float bufferVals[BATCH_SIZE];
uint8_t bufferIndex = 0;

// ——— Capacitance Sensor Settings ————————————————————————————————————
#define UPPER_BOUND   0x4000
#define LOWER_BOUND  (-UPPER_BOUND)
#define CHANNEL       3
#define MEASUREMENT   0

int capdac = 0;
FDC1004 FDC;
float capacitanceValue = 0.0;

// ——— Function Prototypes —————————————————————————————————————————
void printWifiStatus();
void readCapacitance();
void sendBatch();

void setup() {
  Serial.begin(9600);
  while (!Serial) { /* wait for native USB */ }

  // Check for WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module missing!");
    while (true);
  }
  // Firmware check
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade WiFi firmware");
  }

  // Connect to WiFi
  while (status != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(1000);
  }
  printWifiStatus();

  // Init I2C and sensor
  Wire.begin();
}

void loop() {
  // 1) Read one new sample
  readCapacitance();
  bufferVals[bufferIndex++] = capacitanceValue;

  // 2) If buffer is full, POST the batch
  if (bufferIndex >= BATCH_SIZE) {
    sendBatch();
    bufferIndex = 0;
  }

  // 3) Print any server response (for debugging)
  String response = "";
  while (client.available()) {
    response += (char)client.read();
  }
  if (response.length()) {
    Serial.println(response);
  }

  // 4) Delay between samples (adjust as needed)
  delay(100);
}

// ——— Send buffered readings as one JSON POST ———————————————————————
void sendBatch() {
  client.stop();
  if (!client.connect(server, 5000)) {
    Serial.println("Batch upload failed: cannot connect");
    return;
  }

  // Build JSON payload manually:
  String payload = "{\"capacitances\":[";
  for (uint8_t i = 0; i < BATCH_SIZE; i++) {
    payload += String(bufferVals[i], 4);
    if (i < BATCH_SIZE - 1) payload += ",";
  }
  payload += "]}";

  // HTTP POST header
  client.println("POST /test HTTP/1.1");
  client.print  ("Host: "); client.println(server);
  client.println("Content-Type: application/json");
  client.print  ("Content-Length: "); client.println(payload.length());
  client.println("Connection: close");
  client.println();
  // Body
  client.println(payload);

  Serial.print("Sent batch: ");
  Serial.println(payload);
}

// ——— Print WiFi status to Serial ———————————————————————————————————
void printWifiStatus() {
  Serial.print("SSID: ");       Serial.println(WiFi.SSID());
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");       Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

// ——— Read one capacitance measurement into capacitanceValue ——————————
void readCapacitance() {
  FDC.configureMeasurementSingle(MEASUREMENT, CHANNEL, capdac);
  FDC.triggerSingleMeasurement(MEASUREMENT, FDC1004_100HZ);
  delay(10);

  uint16_t raw[2];
  if (!FDC.readMeasurement(MEASUREMENT, raw)) {
    int16_t msb = (int16_t)raw[0];
    int32_t capCalc = (int32_t)457 * msb;
    capCalc /= 1000;
    capCalc += (int32_t)3028 * capdac;
    capacitanceValue = capCalc / 1000.0;  // in pF

    Serial.print(capacitanceValue, 4);
    Serial.println(" pF");

    // auto‐adjust CAPDAC if out of range
    if      (msb >  UPPER_BOUND && capdac < FDC1004_CAPDAC_MAX) capdac++;
    else if (msb <  LOWER_BOUND && capdac > 0)                  capdac--;
  }
}
