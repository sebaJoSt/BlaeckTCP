/*
  SineGeneratorEthernet.ino

  This is a sample sketch to show how to use the BlaeckTCP library to transmit identical sine waves
  from an Arduino with Ethernet Shield (Server) to your PC (Client) every minute (or the user-set interval).

  Circuit:
   Ethernet shield attached to pins 10, 11, 12, 13

  Usage:
    Upload the sketch to your board.
    Open a Telnet Client (e.g. PuTTY) and connect to IP Adress 192.168.1.177 (Port 23)
    Type the following commands and press enter:

    <BLAECK.GET_DEVICES>              Writes the device's information to the PC
    <BLAECK.WRITE_SYMBOLS>            Writes the symbol list to the PC
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
#define MAX_SIGNALS 10

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
  // You can use Ethernet.init(pin) to configure the CS pin
  // Ethernet.init(10);  // Most Arduino shields
  // Ethernet.init(5);   // MKR ETH Shield
  // Ethernet.init(0);   // Teensy 2.0
  // Ethernet.init(20);  // Teensy++ 2.0
  // Ethernet.init(15);  // ESP8266 with Adafruit FeatherWing Ethernet
  // Ethernet.init(33);  // ESP32 with Adafruit FeatherWing Ethernet

  // initialize the Ethernet device
  Ethernet.begin(mac, ip, myDns, gateway, subnet);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial)
  {
    // wait for serial port to connect. Needed for native USB port only
  }

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println();
    Serial.println("Ethernet shield was not found. Sorry, can't run without hardware. :(");
    while (true)
    {
      delay(1); // do nothing, no point running without Ethernet hardware
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

  // Setup BlaeckTCP
  BlaeckTCP.begin(
      MAX_CLIENTS, // Maximal number of allowed clients
      &Serial,     // Serial reference, used for debugging
      MAX_SIGNALS, // Maximal signal count used;
      0b11111101   // Clients permitted to receive data messages; from right to left: client #0, #1, .. , #7
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
