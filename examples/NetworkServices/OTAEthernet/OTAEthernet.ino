/*
  OTAEthernet.ino

  This is a sample sketch to show how to update a BlaeckTCP board over the network
  (over-the-air): once it runs, a new sketch can be uploaded without USB.

  This example requires the ArduinoOTA library (by Juraj Andrassy) to be installed.

  Boards:
    Arduino UNO R4 Minima or WiFi   works as is.
    Arduino Mega 2560               needs the Optiboot bootloader first, see the ArduinoOTA
                                    README, "ATmega support".
    An update has to fit in half of the flash the sketch area has.

  Uploading:
    The board answers network discovery under the name passed to ArduinoOTA.begin(), and
    accepts uploads with the password passed next to it.

  Circuit:
   Ethernet shield attached to pins 10, 11, 12, 13

  created by Sebastian Strobl
  More information on: https://github.com/sebaJoSt/BlaeckTCP
 */

#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoOTA.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_SIGNALS 1

// Two of the shield's sockets go to the update listener and to discovery.
#define MAX_CLIENTS 6

BlaeckTCP BlaeckTCP;

// Seconds since the board started. Drops back to zero after an update, which shows it landed.
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

  // Name, password, and where a received sketch is kept until it replaces this one.
  ArduinoOTA.begin(Ethernet.localIP(), "OTAEthernet", "password", InternalStorage);

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

  BlaeckTCP.DeviceName = "OTA Ethernet";
  BlaeckTCP.DeviceHWVersion = "Arduino Mega 2560 Rev3";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  BlaeckTCP.addSignal("Uptime_s", &uptime);
}

void loop()
{
  uptime = millis() / 1000;

  BlaeckTCP.tick();

  // Answers discovery, and takes an upload when one arrives.
  ArduinoOTA.poll();

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
