/*
  BonjourOTAEthernet.ino

  Requires two libraries, and a third on the Giga:
    EthernetBonjour        the board answers to its host name ("BonjourOTAEthernet" or
                           "BonjourOTAEthernet.local") and announces itself, so upload
                           tools and loggers find it on the network.
    ArduinoOTA             a new sketch can be uploaded to the board over the network.
    Arduino_Portenta_OTA   Giga only: the bootloader applies the new sketch from the QSPI
                           flash.

  Boards:
    Arduino UNO R4 Minima or WiFi   works as is.
    Arduino Mega 2560               needs the Optiboot bootloader first.
    Arduino Giga R1                 needs its QSPI flash partitioned first.
    Both are set up once, as README.md beside this sketch describes. An update has to fit in
    half of the flash the sketch area has; on the Giga it may be almost 5 MB.

  Uploading:
    The board announces itself for network discovery under its host name, and accepts
    uploads with the password passed to ArduinoOTA.begin().

  Names:
    On the local network Bonjour answers "BonjourOTAEthernet" and "BonjourOTAEthernet.local".
    A DHCP server is told a different name: the Ethernet library always sends "WIZnet" plus
    the last three bytes of the MAC address, e.g. WIZnetEFFEED. So on a network whose DNS
    registers DHCP names, that is the name the board answers to there.

  Circuit:
   Ethernet shield attached to pins 10, 11, 12, 13

  created by Sebastian Strobl
  More information on: https://github.com/sebaJoSt/BlaeckTCP
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetBonjour.h>  // Bonjour

#define NO_OTA_PORT  // If Bonjour is used: turns off discovery in ArduinoOTA.h, so only EthernetBonjour answers discovery
#include <ArduinoOTA.h>  // OTA

/*
  OTA: the storage that keeps a received sketch until it replaces this one. A Giga keeps it in a
  file on the QSPI flash beside its processor; the other boards keep it in the second half of
  their own flash.
*/
#if defined(ARDUINO_GIGA)
#include "QspiOtaStorage.h"
QspiOtaStorageClass QspiStorage;
OTAStorage &OtaStorage = QspiStorage;
#else
OTAStorage &OtaStorage = InternalStorage;
#endif

#include "BlaeckTCP.h"

#define HOST_NAME "BonjourOTAEthernet"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_SIGNALS 1

// Two of the shield's sockets go to the update listener and to the host name.
#define MAX_CLIENTS 6

BlaeckTCP BlaeckTCP;

// Seconds since the board started. Drops back to zero after an update, which shows it landed.
unsigned long uptime;

// A flag for whether DHCP gave the address. Only such an address has a lease to renew.
bool leased = false;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// The fallback address, for when no DHCP server answers, e.g. a board cabled straight to a PC.
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

  // A DHCP wait long enough for a managed network to answer. A board with no DHCP server falls back after it.
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

  // Bonjour: the host name the board answers to. This call comes before any other EthernetBonjour call.
  EthernetBonjour.begin(HOST_NAME);

  // Both: announces the update service, so tools browsing for network boards find it.
  EthernetBonjour.addServiceRecord(HOST_NAME "._arduino",
                                   65280,
                                   MDNSServiceTCP,
                                   "\x0d" "ssh_upload=no"
                                   "\x0c" "tcp_check=no"
                                   "\x0f" "auth_upload=yes"
                                   "\x0d" "board=arduino");

  // Bonjour: announces the BlaeckTCP server, so a logger browsing for devices finds it.
  EthernetBonjour.addServiceRecord(HOST_NAME "._blaeck", SERVER_PORT, MDNSServiceTCP);

  // OTA: name, password, and the storage chosen above.
  ArduinoOTA.begin(Ethernet.localIP(), HOST_NAME, "password", OtaStorage);

  Serial.print("BlaeckTCP Server: ");
  Serial.print(HOST_NAME);
  Serial.print(":");
  Serial.print(SERVER_PORT);
  Serial.print(" (");
  Serial.print(Ethernet.localIP());
  Serial.print(":");
  Serial.print(SERVER_PORT);
  Serial.println(")");

  // The name the Ethernet library sends with its DHCP request, which is not the host name:
  // "WIZnet" and the last three bytes of the MAC address. Nothing is sent on the fallback address.
  if (leased)
  {
    char dhcpName[13];
    snprintf(dhcpName, sizeof(dhcpName), "WIZnet%02X%02X%02X", mac[3], mac[4], mac[5]);
    Serial.print("DHCP host name: ");
    Serial.println(dhcpName);
  }

  BlaeckTCP.begin(
      MAX_CLIENTS,
      &Serial,
      MAX_SIGNALS,
      SERVER_PORT  // TCP server port
  );

  BlaeckTCP.DeviceName = HOST_NAME;
#if defined(ARDUINO_GIGA)
  BlaeckTCP.DeviceHWVersion = "Arduino Giga R1";
#else
  BlaeckTCP.DeviceHWVersion = "Arduino Mega 2560 Rev3";
#endif
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  BlaeckTCP.addSignal("Uptime_s", &uptime);
}

void loop()
{
  uptime = millis() / 1000;

  BlaeckTCP.tick();

  // OTA: takes an upload when one arrives.
  ArduinoOTA.poll();

  // Bonjour: answers the host name and discovery. Nothing finds the board without it.
  EthernetBonjour.run();

  // Maintains the DHCP lease.
  if (leased)
  {
    Ethernet.maintain();
  }
}
