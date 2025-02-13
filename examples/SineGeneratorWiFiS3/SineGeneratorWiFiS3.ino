/*
  SineGeneratorWiFiS3.ino

  This is a sample sketch to show how to use the BlaeckTCP library to transmit identical sine waves
  from an Arduino with built-in ESP32-S3 module (Server) to your PC (Client) over WiFi.
  Server and client must be in the same WLAN.

  This example was tested on the Arduino UNO R4 WiFi.

  by Sebastian Strobl
  created: 2023-11-30
  More information on: https://github.com/sebaJoSt/BlaeckTCP
 */

#include <WiFiS3.h>
#include <TelnetPrint.h>
#include "arduino_secrets.h"
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_CLIENTS 8
#define MAX_SIGNALS 10

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

// Signals
float sine;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial)
  {
    // wait for serial port to connect. Needed for native USB port only
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  Serial.println("WiFi connected.");
  Serial.print("IP address: ");

  Serial.print("BlaeckTCP Server: ");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.println(SERVER_PORT);

  // Setup BlaeckTCP
  BlaeckTCP.begin(
      MAX_CLIENTS, // Maximal number of allowed clients
      &Serial,     // Serial reference, used for debugging
      MAX_SIGNALS, // Maximal signal count used;
      0b11111101   // Clients permitted to receive data messages; from right to left: client #0, #1, .. , #7
  );

  BlaeckTCP.DeviceName = "Basic Sine Number Generator";
  BlaeckTCP.DeviceHWVersion = "Arduino UNO R4 WiFi";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  // Add signals to BlaeckTCP
  for (int i = 1; i <= MAX_SIGNALS; i++)
  {
    String signalName = "Sine_";
    BlaeckTCP.addSignal(signalName + i, &sine);
  }

  // Start listening for clients
  TelnetPrint = NetServer(SERVER_PORT);
  TelnetPrint.begin();
}

void loop()
{
  UpdateSineNumbers();

  /*- Keeps watching for commands from TCP clients and transmits the reply messages back to all
     connected clients (data messages only to permitted)
   - Sends data messages to permitted clients (0b11111101) at the user-set interval (<BlAECK.ACTIVATE,..>) */
  BlaeckTCP.tick();
}

void UpdateSineNumbers()
{
  sine = sin(millis() * 0.00005);
}
