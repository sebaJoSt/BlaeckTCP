/*
  BasicEthernet.ino

  This is a sample sketch to show how to use the BlaeckTCP library to transmit data
  from an Arduino with Ethernet Shield (Server) to your PC (Client) every minute (or the user-set interval).

  Circuit:
    Ethernet shield attached to pins 10, 11, 12, 13

  Usage:
    Upload the sketch to your board.
    Open a Telnet Client (e.g. PuTTY) and connect to the IP address printed on the serial monitor (Port 23)
    Type the following commands and press enter:

    <BLAECK.GET_DEVICES>              Writes the device's information to the PC
    <BLAECK.WRITE_SYMBOLS>            Writes the symbol list to the PC
    <BLAECK.WRITE_COMMANDS>           Writes the command list to the PC
    <BLAECK.WRITE_DATA>               Writes the data to the PC
    <BLAECK.ACTIVATE,96,234>          The data is written every 60 seconds (60 000ms)
                                      first Byte:  0b01100000 = 96 DEC
                                      second Byte: 0b11101010 = 234 DEC
                                      Minimum: 0[milliseconds] Maximum: 4 294 967 295[milliseconds]
    <BLAECK.DEACTIVATE>               Stops writing the data every 60s


  created by Sebastian Strobl
  More information on: https://github.com/sebaJoSt/BlaeckTCP
 */

#include <SPI.h>
#include <Ethernet.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_CLIENTS 8

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

// Signals
float randomSmallNumber;
long randomBigNumber;

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

  // Long enough for a managed network to answer; a board with no DHCP server falls back after it.
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

  // Setup BlaeckTCP
  BlaeckTCP.begin(
      MAX_CLIENTS, // Maximal number of allowed clients
      &Serial,     // Serial reference, used for debugging
      2,           // Maximal signal count used;
      SERVER_PORT  // TCP server port
  );

  BlaeckTCP.DeviceName = "Random Number Generator Ethernet";
  BlaeckTCP.DeviceHWVersion = "Arduino Mega 2560 Rev3";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  // Add signals to BlaeckTCP
  BlaeckTCP.addSignal("Small Number", &randomSmallNumber);
  BlaeckTCP.addSignal("Big Number", &randomBigNumber);

  /* On AVR - a Mega with an Ethernet shield, say - wrapping the name in F()
     roughly halves the RAM it costs:
       BlaeckTCP.addSignal(F("Small Number"), &randomSmallNumber);
     A plain literal is copied into RAM at startup AND again into the String
     the signal keeps; F() keeps the literal in flash, so only the String copy
     remains. The ESP32 and other 32-bit targets handle RAM differently and
     gain little. */

  /*Uncomment for fixed interval lock (ms)
    - ignores ACTIVATE/DEACTIVATE while locked */
  // BlaeckTCP.setIntervalMs(60000);
}

void loop()
{
  UpdateRandomNumbers();

  /*- Keeps watching for commands from TCP clients and transmits the reply messages back to all
      connected clients
    - Sends data messages to all clients at the user-set interval (<BlAECK.ACTIVATE,..>) */
  BlaeckTCP.tick();

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
