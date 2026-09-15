/*
  BonjourOTAESP32PoE.ino

  Requires two libraries:
    ESPmDNS      comes with the ESP32 core. The board answers to its host name
                 ("BonjourOTAESP32PoE" or "BonjourOTAESP32PoE.local") and announces itself,
                 so upload tools and loggers find it on the network.
    ArduinoOTA   from the Library Manager, not the one bundled with the ESP32 core. The
                 bundled one has the board fetch a new sketch, which needs the PC to accept
                 an incoming connection; this one listens and lets the PC push, as the
                 Ethernet shield examples do. The bundled one has to be removed first: see
                 README.md beside this sketch.

  Board:
    Olimex ESP32-POE or ESP32-POE-ISO. An update has to fit in one app partition of the
    partition scheme, 1.3 MB with the default one.

  Uploading:
    The board announces itself for network discovery under its host name, and accepts
    uploads on port 65280 with the password passed to ArduinoOTA.begin().

  Names:
    Bonjour answers "BonjourOTAESP32PoE" and "BonjourOTAESP32PoE.local". The same name goes
    to the DHCP server, so on a network whose DNS registers DHCP names the board answers to
    it there as well.

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

// The upload listener is built on the core's WiFi server and client, which serve every
// interface the board has - here that is Ethernet.
#include <WiFi.h>
#include <ESPmDNS.h>

#define NO_OTA_PORT  // ESPmDNS announces the upload service below, so ArduinoOTA does not
#include <ArduinoOTA.h>

#include "BlaeckTCP.h"

#define HOST_NAME "BonjourOTAESP32PoE"

#define EXAMPLE_VERSION "2.0"
#define SERVER_PORT 23
#define MAX_SIGNALS 1
#define MAX_CLIENTS 8

BlaeckTCP BlaeckTCP;

// Seconds since the board started. Drops back to zero after an update, which shows it landed.
unsigned long uptime;

// Used only when no DHCP server answers, e.g. a board cabled straight to a PC.
IPAddress ip(192, 168, 10, 177);
IPAddress dns(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 0, 0);

void onEvent(arduino_event_id_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    // After the interface starts and before DHCP asks, so the DHCP server learns the name too.
    ETH.setHostname(HOST_NAME);
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    Serial.println("Ethernet cable is not connected.");
    break;
  default:
    break;
  }
}

void setup()
{
  // Serial is used for debug output only
  Serial.begin(115200);
  Serial.println();
  Serial.println("Looking for an address...");

  Network.onEvent(onEvent);
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

  // The host name the board answers to.
  MDNS.begin(HOST_NAME);
  MDNS.setInstanceName(HOST_NAME);

  // Announces the update service, so tools browsing for network boards find it.
  MDNS.enableArduino(65280, true);

  // Announces the BlaeckTCP server, so a logger browsing for devices finds it.
  MDNS.addService("blaeck", "tcp", SERVER_PORT);

  // Name, password, and where a received sketch is kept until it replaces this one.
  ArduinoOTA.begin(ETH.localIP(), HOST_NAME, "password", InternalStorage);

  Serial.print("BlaeckTCP Server: ");
  Serial.print(ETH.localIP());
  Serial.print(":");
  Serial.println(SERVER_PORT);

  BlaeckTCP.begin(
      MAX_CLIENTS,
      &Serial,
      MAX_SIGNALS,
      SERVER_PORT  // TCP server port
  );

  BlaeckTCP.DeviceName = HOST_NAME;
  BlaeckTCP.DeviceHWVersion = "ESP32-PoE-ISO Rev.L";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  BlaeckTCP.addSignal("Uptime_s", &uptime);
}

void loop()
{
  uptime = millis() / 1000;

  BlaeckTCP.tick();

  // Takes an upload when one arrives. mDNS and the DHCP lease run on their own.
  ArduinoOTA.poll();
}
