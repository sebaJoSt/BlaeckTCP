/*
  BasicEthernet.ino

  This is a sample sketch to show how to use the BlaeckTCP library to transmit data
  from an Arduino with Ethernet Shield (Server) to your PC (Client), at the interval a host asks for.

  Circuit:
    Ethernet shield attached to pins 10, 11, 12, 13

  Usage:
    Upload the sketch, and open the serial monitor at 115200 baud: it prints the
    board's address.

    Connect Loggbok, or another Blaeck host, to that address on port 23. It asks for
    the data with <BLAECK.ACTIVATE,1000> (one frame a second) and stops it with
    <BLAECK.DEACTIVATE>.

    To watch what the device does, connect a telnet client such as PuTTY to the same
    address and port. It shows connections, the commands that arrive, and anything the
    library refuses. Typing a BLAECK. command there turns it into a host too, and binary
    frames follow.

  created by Sebastian Strobl
  More information on: https://github.com/sebaJoSt/BlaeckTCP
 */

#include <SPI.h>
#include <Ethernet.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
// A W5100 shield has four sockets, and each connection takes a receive buffer in RAM.
#define MAX_CLIENTS 4

// Instantiate a new BlaeckTCP object
BlaeckTCP Blaeck;

// Signals
float randomSmallNumber;
long randomBigNumber;

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
  // You can use Ethernet.init(pin) to configure the CS pin
  // Ethernet.init(10);  // Most Arduino shields
  // Ethernet.init(5);   // MKR ETH Shield
  // Ethernet.init(0);   // Teensy 2.0
  // Ethernet.init(20);  // Teensy++ 2.0
  // Ethernet.init(15);  // ESP8266 with Adafruit FeatherWing Ethernet
  // Ethernet.init(33);  // ESP32 with Adafruit FeatherWing Ethernet

  // Open serial communications (used for debug output only)
  Serial.begin(115200);
  Serial.println();
  Serial.println("Looking for an address...");

  // A DHCP wait long enough for a managed network to answer. A board with no DHCP server falls back after it.
  leased = Ethernet.begin(mac, 30000, 2000) != 0;

  if (!leased)
  {
    Ethernet.begin(mac, ip, myDns, gateway, subnet);
  }

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println("Ethernet shield was not found. Sorry, can't run without hardware. :(");
    while (true)
    {
      delay(1); // do nothing, no point running without Ethernet hardware
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

  Serial.print("BlaeckTCP Server: ");
  Serial.print(Ethernet.localIP());
  Serial.print(":");
  Serial.println(SERVER_PORT);

  // Setup BlaeckTCP. The library reports on the terminal connections.
  Blaeck.begin(SERVER_PORT)
      .withClients(MAX_CLIENTS)
      .withSignals(2)
      .withDebugStream(&Blaeck.Terminal);

  Blaeck.DeviceName = "Random Number Generator Ethernet";
  Blaeck.DeviceHWVersion = "Arduino Mega 2560 Rev3";
  Blaeck.DeviceFWVersion = EXAMPLE_VERSION;

  // Add signals to BlaeckTCP
  Blaeck.addSignal(F("Small Number"), &randomSmallNumber);
  Blaeck.addSignal(F("Big Number"), &randomBigNumber);
}

void loop()
{
  UpdateRandomNumbers();

  // Handles connections and commands, and sends the signals when the interval is up.
  Blaeck.tick();

  // Maintains the DHCP lease.
  if (leased)
  {
    Ethernet.maintain();
  }
}

void UpdateRandomNumbers()
{
  // Random small number from 0.00 to 10.00
  randomSmallNumber = random(1001) / 100.0;

  // Random big number from 2 000 000 000 to 2 100 000 000
  randomBigNumber = random(2000000000, 2100000001);
}
