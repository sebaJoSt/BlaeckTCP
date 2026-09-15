/*
  BonjourOTAEthernetGiga.ino

  Requires three libraries:
    EthernetBonjour        the board answers to its host name ("BonjourOTAEthernetGiga" or
                           "BonjourOTAEthernetGiga.local") and announces itself, so upload
                           tools and loggers find it on the network.
    ArduinoOTA             a new sketch can be uploaded to the board over the network.
    Arduino_Portenta_OTA   the bootloader applies the new sketch from the QSPI flash.

  The board needs setting up once before its first over-the-air upload: see README.md
  beside this sketch.

  Uploading:
    The board announces itself for network discovery under its host name, and accepts
    uploads with the password passed to ArduinoOTA.begin().

  Names:
    On the local network Bonjour answers "BonjourOTAEthernetGiga" and
    "BonjourOTAEthernetGiga.local". A DHCP server is told a different name: the Ethernet
    library always sends "WIZnet" plus the last three bytes of the MAC address, e.g.
    WIZnetEFFEED. So on a network whose DNS registers DHCP names, that is the name the
    board answers to there.

  Circuit:
   Ethernet shield attached to pins 10, 11, 12, 13

  created by Sebastian Strobl
  More information on: https://github.com/sebaJoSt/BlaeckTCP
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetBonjour.h>

#define NO_OTA_PORT  // If Bonjour is used: turns off discovery in ArduinoOTA.h, so only EthernetBonjour answers discovery
#include <ArduinoOTA.h>

// Where an update is kept on this board, which is a file on the QSPI flash beside the
// processor rather than a second copy in the flash the sketch runs from.
#include "QspiOtaStorage.h"

#include "BlaeckTCP.h"

#define HOST_NAME "BonjourOTAEthernetGiga"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_SIGNALS 1

// Two of the shield's sockets go to the update listener and to the host name.
#define MAX_CLIENTS 6

BlaeckTCP BlaeckTCP;
QspiOtaStorageClass QspiStorage;

// Seconds since the board started. Drops back to zero after an update, which shows it landed.
unsigned long uptime;

// Whether an address was leased, which decides whether there is a lease to renew.
bool leased = false;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// Used only when no DHCP server answers, e.g. a board cabled straight to a PC.
IPAddress ip(192, 168, 10, 177);
IPAddress myDns(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 0, 0);

void setup()
{
  // Serial is used for debug output only
  Serial.begin(115200);
  Serial.println();
  Serial.println("Looking for an address...");

  // Long enough for a managed network to answer; a board with no DHCP server falls back after it.
  leased = Ethernet.begin(mac, 30000, 2000) != 0;

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
  EthernetBonjour.begin(HOST_NAME);

  // Announces the update service, so tools browsing for network boards find it.
  EthernetBonjour.addServiceRecord(HOST_NAME "._arduino",
                                   65280,
                                   MDNSServiceTCP,
                                   "\x0d" "ssh_upload=no"
                                   "\x0c" "tcp_check=no"
                                   "\x0f" "auth_upload=yes"
                                   "\x0d" "board=arduino");

  // Announces the BlaeckTCP server, so a logger browsing for devices finds it.
  EthernetBonjour.addServiceRecord(HOST_NAME "._blaeck", SERVER_PORT, MDNSServiceTCP);

  // Name, password, and where a received sketch is kept until it replaces this one.
  ArduinoOTA.begin(Ethernet.localIP(), HOST_NAME, "password", QspiStorage);

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

  BlaeckTCP.DeviceName = HOST_NAME;
  BlaeckTCP.DeviceHWVersion = "Arduino Giga R1";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  BlaeckTCP.addSignal("Uptime_s", &uptime);
}

void loop()
{
  uptime = millis() / 1000;

  BlaeckTCP.tick();

  // Takes an upload when one arrives.
  ArduinoOTA.poll();

  // Answers the host name and discovery. Nothing finds the board without it.
  EthernetBonjour.run();

  // Maintains the DHCP lease.
  if (leased)
  {
    Ethernet.maintain();
  }
}
