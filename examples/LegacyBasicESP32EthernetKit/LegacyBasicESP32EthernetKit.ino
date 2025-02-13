/*
  LegacyBasicESP32EthernetKit.ino

  This example currently only works with ESP32 2.x firmware (Board: ESP32 Dev Module),

  This is a sample sketch to show how to use the BlaeckTCP library to transmit data
  from the ESP32-EthernetKit_A_V1.2 (Server) to your PC (Client) every minute (or the user-set interval).

  The ESP32-EthernetKit_A_V1.2 contains the ESP32-WROVER-E module and the IP101GRI, a Single Port 10/100 Fast Ethernet Transceiver (PHY).

 Setup:

    Upload the sketch to your board.

  Usage:

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

#include <ETH.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_CLIENTS 8

// Pin definitions for ESP32 Ethernet Kit_A with IP101GRI
#define ETH_ADDR 1
#define ETH_POWER_PIN 5
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18
#define ETH_TYPE ETH_PHY_IP101

// Enter a static IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

static bool eth_connected = false;

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

// Signals
float randomSmallNumber;
long randomBigNumber;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  delay(3000);

  WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE);
  ETH.config(ip, gateway, subnet, myDns);

  Serial.print("BlaeckTCP Server: ");
  Serial.print(ETH.localIP());
  Serial.print(":");
  Serial.println(SERVER_PORT);

  // Setup BlaeckTCP
  BlaeckTCP.begin(
      MAX_CLIENTS, // Maximal number of allowed clients
      &Serial,     // Serial reference, used for debugging
      2,           // Maximal signal count used;
      0b11111101   // Clients permitted to receive data messages; from right to left: client #0, #1, .. , #7
  );

  BlaeckTCP.DeviceName = "Random Number Generator";
  BlaeckTCP.DeviceHWVersion = "ESP32-EthernetKit_A_V1.2";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  // Add signals to BlaeckTCP
  BlaeckTCP.addSignal("Small Number", &randomSmallNumber);
  BlaeckTCP.addSignal("Big Number", &randomBigNumber);

  /*Uncomment this function for initial settings
    first parameter: timedActivated
    second parameter: timedInterval_ms */
  // BlaeckTCP.setTimedData(true, 60000);

  // Start listening for clients
  TelnetPrint = NetServer(SERVER_PORT);
  TelnetPrint.begin();
}

void loop()
{

  UpdateRandomNumbers();

  /*- Keeps watching for commands from TCP clients and transmits the reply messages back to all
      connected clients (data messages only to permitted)
    - Sends data messages to permitted clients (0b11111101) at the user-set interval (<BlAECK.ACTIVATE,..>) */
  BlaeckTCP.tick();
}

void UpdateRandomNumbers()
{
  // Random small number from 0.00 to 10.00
  randomSmallNumber = random(1001) / 100.0;

  // Random big number from 2 000 000 000 to 2 100 000 000
  randomBigNumber = random(2000000000, 2100000001);
}

void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    Serial.println("ETH Started");
    // set eth hostname here
    ETH.setHostname("esp32-ethernet");
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    Serial.println("ETH Connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    Serial.print("ETH MAC: ");
    Serial.print(ETH.macAddress());
    Serial.print(", IPv4: ");
    Serial.print(ETH.localIP());
    if (ETH.fullDuplex())
    {
      Serial.print(", FULL_DUPLEX");
    }
    Serial.print(", ");
    Serial.print(ETH.linkSpeed());
    Serial.println("Mbps");
    eth_connected = true;
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    Serial.println("ETH Disconnected");
    eth_connected = false;
    break;
  case ARDUINO_EVENT_ETH_STOP:
    Serial.println("ETH Stopped");
    eth_connected = false;
    break;
  default:
    break;
  }
}
