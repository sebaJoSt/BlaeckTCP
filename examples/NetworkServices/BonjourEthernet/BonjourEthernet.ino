/*
  BonjourEthernet.ino

  This is a sample sketch to show how to reach a BlaeckTCP board by its host name
  ("BonjourEthernet" or "BonjourEthernet.local") instead of by its address, using Bonjour/mDNS.

  This example requires the EthernetBonjour library to be installed.

  Circuit:
   Ethernet shield attached to pins 10, 11, 12, 13

  created by Sebastian Strobl
  More information on: https://github.com/sebaJoSt/BlaeckTCP
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetBonjour.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_SIGNALS 1

// One of the shield's sockets goes to answering the host name.
#define MAX_CLIENTS 7

BlaeckTCP BlaeckTCP;

// A sign of life: seconds since the board started.
unsigned long uptime;

// Whether an address was leased, which decides whether there is a lease to renew.
bool leased = false;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// Used only when no DHCP server answers, e.g. a board cabled straight to a PC.
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

void setup()
{
  // Serial is used for debug output only
  Serial.begin(115200);
  Serial.println();
  Serial.println("Looking for an address...");

  // A short DHCP timeout, so a board with no DHCP server falls back quickly.
  leased = Ethernet.begin(mac, 8000, 2000) != 0;

  if (!leased)
  {
    Ethernet.begin(mac, ip, myDns, gateway, subnet);
  }

  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println("Ethernet shield was not found. Sorry, can't run without hardware. :(");
    while (true)
    {
      delay(1);
    }
  }

  // Ethernet.begin() returns before the link has finished coming up, so asking
  // straight away reports a connected cable as unplugged. Wait for it, briefly.
  unsigned long settle = millis() + 2000;
  while (Ethernet.linkStatus() == LinkOFF && millis() < settle)
  {
    delay(50);
  }

  if (Ethernet.linkStatus() == LinkOFF)
  {
    Serial.println("Ethernet cable is not connected.");
  }

  // The host name the board answers to. Before any other EthernetBonjour call.
  EthernetBonjour.begin("BonjourEthernet");

  Serial.print("BlaeckTCP Server: ");
  Serial.print(Ethernet.localIP());
  Serial.print(":");
  Serial.println(SERVER_PORT);

  BlaeckTCP.begin(
      MAX_CLIENTS,
      &Serial,
      MAX_SIGNALS,
      SERVER_PORT  // TCP server port
  );

  BlaeckTCP.DeviceName = "Bonjour Ethernet";
  BlaeckTCP.DeviceHWVersion = "Arduino Mega 2560 Rev3";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  BlaeckTCP.addSignal("Uptime_s", &uptime);
}

void loop()
{
  uptime = millis() / 1000;

  BlaeckTCP.tick();

  // Answers the host name lookups. Nothing resolves the name without it.
  EthernetBonjour.run();

  /* Renews the lease when it is due, and only where one was given.

     Not "does nothing on the fixed address", which is what this used to say: after a DHCP
     attempt that nobody answered, the library still holds what it set up for it, and
     working on that here stops Bonjour answering to its name - on a board cabled straight
     to a PC, which is exactly where no DHCP server is. */
  if (leased)
  {
    Ethernet.maintain();
  }
}
