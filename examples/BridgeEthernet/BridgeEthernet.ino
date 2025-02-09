/*
  BridgeEthernet.ino

  This is a sample sketch to show how to use the BlaeckTCP library in Bridge mode.
  
  Circuit:

     PC
     |
     |----------- Ethernet-Cable
     |
  -------------------------
  |  BLAECKTCP BRIDGE     |
  |  Uno or Mega with     |
  |  Ethernet shield      |
  |  attached to pins     |
  |  10, 11, 12, 13       |
  |  (Running             |
  |  BridgeEthernet.ino)  |
  -------------------------
           |  tx     |  rx
           |         |
           |         |-------- Serial connection, crossed lines (tx-rx, rx-tx)
           |  rx     |  tx
  --------------------------
  |  BLAECKSERIAL DEVICE   |
  |  Uno or Mega running   |
  |  your Sketch with      |
  |  BlaeckSerial          |
  |  (e.g.:                |
  |  SineGeneratorBasic.ino|
  |  Example from          |
  |  BlaeckSerial library) |
  --------------------------

  Usage:
    Make sure the baudrates match on BLAECKTCP BRIDGE and BLAECKSERIAL DEVICE!
    Upload the sketches to your boards.
    
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

#define MAX_CLIENTS 8
#define SERVER_PORT 23

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

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

  // In this example Serial1 of the Mega is used to bridge the Signals (Pin 18: TX1, Pin 19: RX1)
  Serial1.begin(115200);

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
  BlaeckTCP.beginBridge(
      MAX_CLIENTS, // Maximal number of allowed clients
      &Serial,     // Serial reference, only used for debugging
      &Serial1     // Bridge Serial reference; connects to the BlaeckSerial device
  );

  // Start listening for clients
  TelnetPrint = NetServer(SERVER_PORT);
  TelnetPrint.begin();
}

void loop()
{
 
  BlaeckTCP.tickBridge();

}
