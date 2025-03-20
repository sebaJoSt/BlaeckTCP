/*
  BasicESP32C6Bug.ino

  This is a sample sketch to show how to use the BlaeckTCP library to transmit data
  from the ESP32-C6-Bug (V2.1.0) + ESP32-BUG-ETH (V1.0.0) to your PC (Client) every minute
  (or the user-set interval).

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
#include <SPI.h>
#include "BlaeckTCP.h"

#define ETH_TYPE ETH_PHY_W5500
#define ETH_ADDR 1
#define ETH_CS 5
#define ETH_IRQ 4
#define ETH_RST -1

// SPI pins
#define ETH_SPI_SCK 6
#define ETH_SPI_MISO 2
#define ETH_SPI_MOSI 7

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_CLIENTS 8

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

// Enter a static IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
IPAddress ip(192, 168, 1, 177);
IPAddress dns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Signals
float randomSmallNumber;
long randomBigNumber;

void onEvent(arduino_event_id_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    Serial.println("ETH Started");
    ETH.setHostname("ESP32-ETH01");
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    Serial.println("ETH Connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    Serial.print("ETH MAC: ");
    Serial.print(ETH.macAddress());
    Serial.print(", IPv4: ");
    Serial.print(ETH.localIP());
    Serial.print(", ");
    Serial.print(ETH.subnetMask());
    Serial.print(", ");
    Serial.println(ETH.gatewayIP());
    Serial.print("BlaeckTCP Server: ");
    Serial.print(ETH.localIP());
    Serial.print(":");
    Serial.println(SERVER_PORT);
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    Serial.println("ETH Disconnected");
    break;
  case ARDUINO_EVENT_ETH_STOP:
    Serial.println("ETH Stopped");
    break;
  default:
    break;
  }
}

void setup()
{
  Serial.begin(9600);
  delay(500);

  // Register ETH event handler
  Network.onEvent(onEvent);

  // Initialize ETH
  SPI.begin(ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI);
  ETH.begin(ETH_TYPE, ETH_ADDR, ETH_CS, ETH_IRQ, ETH_RST, SPI);

  // Configure static IP
  ETH.config(ip, gateway, subnet, dns);

  // Setup BlaeckTCP
  BlaeckTCP.begin(
      MAX_CLIENTS,
      &Serial,
      2,
      0b11111101);

  BlaeckTCP.DeviceName = "Random Number Generator";
  BlaeckTCP.DeviceHWVersion = "ESP32-C6-Bug V2.1.0";
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
  BlaeckTCP.tick();
}

void UpdateRandomNumbers()
{
  randomSmallNumber = random(1001) / 100.0;
  randomBigNumber = random(2000000000, 2100000001);
}