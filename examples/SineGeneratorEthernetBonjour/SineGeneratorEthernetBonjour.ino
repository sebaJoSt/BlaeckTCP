/*
 SineGeneratorEthernetBonjour.ino

 This sketch is based on SineGeneratorEthernet.ino with the addition of Bonjour/mDNS service.
 Bonjour/mDNS service allows clients to connect using the hostname ("SineNumberGenerator" or
 "SineNumberGenerator.local" in this example) instead of the IP address.

 This example requires EthernetBonjour Libray to be installed.

 Circuit:
   Ethernet shield attached to pins 10, 11, 12, 13

 created by Sebastian Strobl
 More information on: https://github.com/sebaJoSt/BlaeckTCP
*/

#include <SPI.h>
#include <Ethernet.h>
#include "BlaeckTCP.h"
#include <EthernetBonjour.h>

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_CLIENTS 8
#define MAX_SIGNALS 10

BlaeckTCP BlaeckTCP;

float sine;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 10, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

void setup()
{
  Ethernet.begin(mac, ip, myDns, gateway, subnet);

  // Initialize the Bonjour/MDNS library. You can now reach or ping this
  // Arduino via the host name "SineNumberGenerator" or "SineNumberGenerator.local",
  // provided that your operating system is Bonjour-enabled (such as MacOS X).
  // Always call this before any other method!
  EthernetBonjour.begin("SineNumberGenerator");

  Serial.begin(9600);
  while (!Serial)
  {
  }

  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println();
    Serial.println("Ethernet shield was not found. Sorry, can't run without hardware. :(");
    while (true)
    {
      delay(1);
    }
  }

  Serial.println();
  if (Ethernet.linkStatus() == LinkOFF)
  {
    Serial.println("Ethernet cable is not connected.");
  }

  Serial.print("BlaeckTCP Server: ");
  Serial.print(Ethernet.localIP());
  Serial.print(":");
  Serial.println(SERVER_PORT);

  BlaeckTCP.begin(
      MAX_CLIENTS,
      &Serial,
      MAX_SIGNALS,
      0b11111101);

  BlaeckTCP.DeviceName = "Basic Sine Number Generator";
  BlaeckTCP.DeviceHWVersion = "Arduino Mega 2560 Rev3";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  for (int i = 1; i <= MAX_SIGNALS; i++)
  {
    String signalName = "Sine_";
    BlaeckTCP.addSignal(signalName + i, &sine);
  }

  TelnetPrint = NetServer(SERVER_PORT);
  TelnetPrint.begin();
}

void loop()
{
  UpdateSineNumbers();

  BlaeckTCP.tick();

  // This actually runs the Bonjour module. YOU HAVE TO CALL THIS PERIODICALLY,
  // OR NOTHING WILL WORK! Preferably, call it once per loop().
  EthernetBonjour.run();
}

void UpdateSineNumbers()
{
  sine = sin(millis() * 0.00005);
}
