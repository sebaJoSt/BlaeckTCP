/*
  BridgeESP32PoE.ino

  Puts a BlaeckSerial device on the network: an Olimex ESP32-PoE passes bytes between one
  TCP connection and a UART, unchanged in both directions. A host connects to the bridge
  as if the device itself were on the network.

  This sketch doesn't use the BlaeckTCP library. The device behind it runs BlaeckSerial,
  and the bridge only carries its bytes.

  Circuit:

     PC
     |
     |----------- Ethernet cable
     |
  ---------------------------
  |  BRIDGE                 |
  |  ESP32-PoE-ISO Rev. L   |
  |  (BridgeESP32PoE.ino)   |
  ---------------------------
         | TX (BRIDGE_TX_PIN)   | RX (BRIDGE_RX_PIN)
         |                      |
         |                      |-------- Serial connection, crossed (TX-RX, RX-TX)
         | RX                   | TX
  ---------------------------
  |  BLAECKSERIAL DEVICE    |
  |  e.g. SineGeneratorBasic|
  |  from BlaeckSerial      |
  ---------------------------

  Usage:
    Set the same baudrate on the bridge (BRIDGE_BAUD) and in the device's Serial.begin().
    Upload both sketches, and open the bridge's serial monitor at 115200 baud: it prints
    the address.

    Connect Loggbok, or another Blaeck host, to that address on port 23. The bridge takes
    one connection at a time; a second one is closed straight away.

  created by Sebastian Strobl
  More information on: https://github.com/sebaJoSt/BlaeckTCP
 */

// The PHY pins come from the board definition: select the board as OLIMEX ESP32-PoE.
#include <ETH.h>
// The core's WiFi server and client serve every interface, Ethernet included.
#include <WiFi.h>

#define SERVER_PORT 23

// The bridge link runs on its own UART, not on Serial. Serial is the USB port and carries
// this sketch's messages; sharing it would send them to the device too.
// CHECK THESE AGAINST YOUR BOARD - they must be pins the Ethernet PHY doesn't use. The
// defaults are on the UEXT connector of the ESP32-PoE-ISO.
#define BRIDGE_RX_PIN 36
#define BRIDGE_TX_PIN 4
#define BRIDGE_BAUD 115200

// The fallback address, for when no DHCP server answers, e.g. a board cabled straight to a PC.
IPAddress ip(192, 168, 10, 177);
IPAddress dns(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 0, 0);

WiFiServer server(SERVER_PORT);
WiFiClient client;

void onEvent(arduino_event_id_t event)
{
  if (event == ARDUINO_EVENT_ETH_START)
    ETH.setHostname("BridgeESP32PoE");
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  // The UART the BlaeckSerial device is wired to.
  Serial1.begin(BRIDGE_BAUD, SERIAL_8N1, BRIDGE_RX_PIN, BRIDGE_TX_PIN);

  Serial.println("Looking for an address...");
  Network.onEvent(onEvent);
  ETH.begin();

  // A DHCP answer can take a while on a managed network. A board with no DHCP server falls back.
  unsigned long waitUntil = millis() + 30000;
  while (!ETH.hasIP() && millis() < waitUntil)
    delay(50);
  if (!ETH.hasIP())
    ETH.config(ip, gateway, subnet, dns);

  server.begin();
  server.setNoDelay(true);

  Serial.print("Bridge: ");
  Serial.print(ETH.localIP());
  Serial.print(":");
  Serial.println(SERVER_PORT);
}

void loop()
{
  AcceptConnection();

  if (!client.connected())
    return;

  // Network to device.
  uint8_t buffer[128];
  int n = client.available();
  if (n > 0)
  {
    n = client.read(buffer, min(n, (int)sizeof(buffer)));
    if (n > 0)
      Serial1.write(buffer, n);
  }

  // Device to network.
  n = Serial1.available();
  if (n > 0)
  {
    n = Serial1.readBytes(buffer, min(n, (int)sizeof(buffer)));
    if (n > 0)
      client.write(buffer, n);
  }
}

// One connection at a time, so two hosts can't mix their commands on the UART.
void AcceptConnection()
{
  WiFiClient incoming = server.accept();
  if (!incoming)
    return;

  if (client.connected())
  {
    incoming.stop();
    return;
  }

  client = incoming;
  Serial.print("Connected: ");
  Serial.println(client.remoteIP());
}
