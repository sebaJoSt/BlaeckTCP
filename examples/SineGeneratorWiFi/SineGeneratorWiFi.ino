/*
  SineGeneratorWiFi.ino

  This is a sample sketch to show how to use the BlaeckTCP library to transmit identical sine waves
  from the Arduino board to your PC over WiFi.

  This example was tested on ESP32. It should be easily adoptable to ESP8266.

 created by Sebastian Strobl
 */

#include <WiFi.h>
#include <WiFiMulti.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define MAX_CLIENTS 8
#define SERVER_PORT 23
#define MAX_SIGNALS 10

WiFiMulti wifiMulti;
const char *ssid = "**********";
const char *password = "**********";

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

  wifiMulti.addAP(ssid, password);

  Serial.println();
  Serial.println("Connecting Wifi..");
  for (int loops = 10; loops > 0; loops--)
  {
    if (wifiMulti.run() == WL_CONNECTED)
    {
      Serial.println("WiFi connected.");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      break;
    }
    else
    {
      Serial.println(loops);
      delay(1000);
    }
  }
  if (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.println("WiFi connect failed.");
    delay(1000);
    ESP.restart();
  }

  Serial.print("BlaeckTCP Server: ");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.println(SERVER_PORT);

  // Setup BlaeckTCP
  BlaeckTCP.begin(
      MAX_CLIENTS, // Maximal number of allowed clients
      &Serial,     // Serial reference, used for debugging
      MAX_SIGNALS, // Maximal signal count used;
      0b11111101   // Clients allowed to receive Blaeck Data; from right to left: client #0, #1, .. , #7
  );

  BlaeckTCP.DeviceName = "Basic Sine Number Generator";
  BlaeckTCP.DeviceHWVersion = "Arduino Mega 2560 Rev3";
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
  TelnetPrint.setNoDelay(true);
}

void loop()
{
  UpdateSineNumbers();

  /*Keeps watching for commands from TCP client and
    transmits the data back to client at the user-set interval*/
  BlaeckTCP.tick();
}

void UpdateSineNumbers()
{
  sine = sin(millis() * 0.00005);
}
