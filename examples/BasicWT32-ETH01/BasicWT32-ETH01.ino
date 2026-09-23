/*
  BasicWT32-ETH01.ino

  This is a sample sketch to show how to use the BlaeckTCP library to transmit data
  from the WT32-ETH01 V1.4 board (Server) to your PC (Client), at the interval a host asks for.

  Setup:
    Upload the sketch to your board.

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

#include <ETH.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_CLIENTS 8

// ETH pins for WT32-ETH01
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#define ETH_PHY_MDC 23
#define ETH_PHY_MDIO 18
#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN

// Instantiate a new BlaeckTCP object
BlaeckTCP Blaeck;

// Signals
float randomSmallNumber;
long randomBigNumber;

// The fallback address, for when no DHCP server answers, e.g. a board cabled straight to a PC.
IPAddress ip(192, 168, 10, 177);
IPAddress dns(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 0, 0);

void onEvent(arduino_event_id_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    Serial.println("ETH Started");
    ETH.setHostname("BasicWT32-ETH01");
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
  Serial.begin(115200);
  delay(500);

  // Power up ETH PHY
  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);
  delay(100);

  // Register ETH event handler
  Network.onEvent(onEvent);

  // Initialize ETH
  ETH.begin();

  // A DHCP answer can take a while on a managed network. A board with no DHCP server falls back.
  unsigned long waitUntil = millis() + 30000;
  while (!ETH.hasIP() && millis() < waitUntil)
  {
    delay(50);
  }

  if (!ETH.hasIP())
  {
    ETH.config(ip, gateway, subnet, dns);
  }

  // Setup BlaeckTCP. The library reports on the terminal connections.
  Blaeck.begin(SERVER_PORT)
      .withClients(MAX_CLIENTS)
      .withSignals(2)
      .withDebugStream(&Blaeck.Terminal);

  Blaeck.DeviceName = "Random Number Generator WT32-ETH01";
  Blaeck.DeviceHWVersion = "WT32-ETH01 V1.4";
  Blaeck.DeviceFWVersion = EXAMPLE_VERSION;

  // Add signals to BlaeckTCP
  Blaeck.addSignal(F("Small Number"), &randomSmallNumber);
  Blaeck.addSignal(F("Big Number"), &randomBigNumber);
}

void loop()
{
  UpdateRandomNumbers();
  Blaeck.tick();
}

void UpdateRandomNumbers()
{
  randomSmallNumber = random(1001) / 100.0;
  randomBigNumber = random(2000000000, 2100000001);
}
