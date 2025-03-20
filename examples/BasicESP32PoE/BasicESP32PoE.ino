/*
  BasicESP32PoE.ino

  This is a sample sketch to show how to use the BlaeckTCP library to transmit data
  from the Olimex ESP32-PoE-ISO Rev.L board (Server) to your PC (Client) every minute (or the user-set interval).

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

// Important to be defined BEFORE including ETH.h for ETH.begin() to work.
// Example RMII LAN8720 (Olimex, etc.)
#ifndef ETH_PHY_MDC
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#if CONFIG_IDF_TARGET_ESP32
#define ETH_PHY_ADDR 0
#define ETH_PHY_MDC 23
#define ETH_PHY_MDIO 18
// #define ETH_PHY_POWER -1
// #define ETH_CLK_MODE  ETH_CLOCK_GPIO0_IN
#define ETH_PHY_POWER 12
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#elif CONFIG_IDF_TARGET_ESP32P4
#define ETH_PHY_ADDR 0
#define ETH_PHY_MDC 31
#define ETH_PHY_MDIO 52
#define ETH_PHY_POWER 51
#define ETH_CLK_MODE EMAC_CLK_EXT_IN
#endif
#endif

#include <ETH.h>
#include "BlaeckTCP.h"

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
  ETH.begin();

  // Configure static IP
  ETH.config(ip, gateway, subnet, dns);

  // Setup BlaeckTCP
  BlaeckTCP.begin(
      MAX_CLIENTS,
      &Serial,
      2,
      0b11111101);

  BlaeckTCP.DeviceName = "Random Number Generator";
  BlaeckTCP.DeviceHWVersion = "ESP32-PoE-ISO Rev.L";
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