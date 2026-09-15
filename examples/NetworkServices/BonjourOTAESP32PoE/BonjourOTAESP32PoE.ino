/*
  BonjourOTAESP32PoE.ino

  Uses two libraries that come with the ESP32 core:
    ESPmDNS      the board answers to its host name ("BonjourOTAESP32PoE" or
                 "BonjourOTAESP32PoE.local") and announces itself, so upload tools
                 and loggers find it on the network.
    ArduinoOTA   a new sketch can be uploaded to the board over the network.

  Board:
    Olimex ESP32-POE or ESP32-POE-ISO. An update has to fit in one app partition of the
    partition scheme, 1.3 MB with the default one.

  Uploading:
    The board announces itself for network discovery under its host name, and accepts
    uploads with the password passed to ArduinoOTA.setPassword(). This is the ESP32 core's
    own upload, on port 3232, not the one the Ethernet shield examples use.

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
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

#include "BlaeckTCP.h"

#define HOST_NAME "BonjourOTAESP32PoE"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_SIGNALS 1
#define MAX_CLIENTS 8

BlaeckTCP BlaeckTCP;

// Seconds since the board started. Drops back to zero after an update, which shows it landed.
unsigned long uptime;

// Used only when no DHCP server answers, e.g. a board cabled straight to a PC.
IPAddress ip(192, 168, 10, 178);
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

  // Name and password. Also starts mDNS, which answers the host name and announces the update service.
  ArduinoOTA.setHostname(HOST_NAME);
  ArduinoOTA.setPassword("password");
  ArduinoOTA.begin();

  // Announces the BlaeckTCP server under the host name, so a logger browsing for devices finds it.
  MDNS.setInstanceName(HOST_NAME);
  MDNS.addService("blaeck", "tcp", SERVER_PORT);

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
  ArduinoOTA.handle();
}
