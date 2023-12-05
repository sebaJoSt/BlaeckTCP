/*
  BasicWT32-ETH01.ino

  This is a sample sketch to show how to use the BlaeckTCP library to transmit data
  from the WT32-ETH01 V1.4 board (Server) to your PC (Client) every minute (or the user-set interval).

 Setup:

    - Install the additional library WebServer_WT32_ETH01
    - Upload the sketch to your board.

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

#include <WebServer_WT32_ETH01.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define MAX_CLIENTS 8
#define SERVER_PORT 23

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

// Signals
float randomSmallNumber;
long randomBigNumber;

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
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  delay(3000);

  WT32_ETH01_onEvent();

  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER);
  ETH.config(ip, gateway, subnet, myDns);

  WT32_ETH01_waitForConnect();

  Serial.print("BlaeckTCP Server: ");
  Serial.print(ETH.localIP());
  Serial.print(":");
  Serial.println(SERVER_PORT);

  // Setup BlaeckTCP
  BlaeckTCP.begin(
      MAX_CLIENTS, // Maximal number of allowed clients
      &Serial,     // Serial reference, used for debugging
      2,           // Maximal signal count used;
      0b11111101   // Clients allowed to receive Blaeck Data; from right to left: client #0, #1, .. , #7
  );

  BlaeckTCP.DeviceName = "Random Number Generator";
  BlaeckTCP.DeviceHWVersion = "WT32-ETH01 V1.4";
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

  /*Keeps watching for commands from TCP client and
     transmits the data back to client at the user-set interval*/
  BlaeckTCP.tick();
}

void UpdateRandomNumbers()
{
  // Random small number from 0.00 to 10.00
  randomSmallNumber = random(1001) / 100.0;

  // Random big number from 2 000 000 000 to 2 100 000 000
  randomBigNumber = random(2000000000, 2100000001);
}
