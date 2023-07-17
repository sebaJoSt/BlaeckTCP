/*
  SineGeneratorWiFi.ino

  This is a sample sketch to show how to use the BlaeckTCP library to transmit identical sine waves
  from the Arduino board to your PC.

 created by Sebastian Strobl
 */

#include <SPI.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define MAX_CLIENTS 8
#define SERVER_PORT 23
#define MAX_SIGNALS 10

WiFiMulti wifiMulti;
const char *ssid = "iPhone von Sebastian";
const char *password = "abcd12347";

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

// Signals
float sine;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial)
  {
    // wait for serial port to connect. Needed for native USB port only
  }

  wifiMulti.addAP(ssid, password);
  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

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
  // Create a server that listens for incoming connections on the specified port.
  BlaeckTCP.begin(
      SERVER_PORT, // Port to listen on
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
