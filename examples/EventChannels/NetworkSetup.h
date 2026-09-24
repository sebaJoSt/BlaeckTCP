/*
  NetworkSetup.h

  Gets the board online, for Basic and every topic example in BlaeckTCP. The same file is in
  each of them, and CI fails if the copies differ. Edit examples/Basic/NetworkSetup.h,
  then run extras/scripts/syncnetwork.py to update the other copies.

  In the sketch, before the other includes:

    #define HOST_NAME "WaveformGenerator"     // optional, the name the board answers to
    #define NETWORK_MAC "DE:AD:BE:EF:FE:ED"   // optional, the shield's address (Mega, Giga)
    #define NETWORK_WITH_SERVICES             // optional, OTA updates and Bonjour
    #include "NetworkSetup.h"

  then networkBegin(SERVER_PORT) in setup() before Blaeck.begin(), and networkLoop() in
  loop(). Call Serial.begin() first: networkBegin() waits up to three seconds for the
  monitor before printing startup diagnostics, then continues even without a computer.
  Blaeck supplies the hardware name from the selected build target.

  Boards:
    Arduino Mega 2560      Ethernet shield
    Arduino Giga R1        Ethernet shield
    Olimex ESP32-PoE       built-in Ethernet
    WT32-ETH01             built-in Ethernet

  Each board first asks for an address over DHCP, waiting up to 30 s, and takes the fixed
  address below if none comes, e.g. when cabled straight to a PC.

  Names: with NETWORK_WITH_SERVICES, Bonjour answers to HOST_NAME and HOST_NAME.local on the
  local network. A DHCP server learns HOST_NAME from the ESP32 boards, but the Ethernet
  library on the Mega and the Giga always sends "WIZnet" plus the last three bytes of the
  MAC address, e.g. WIZnetEFFEED. On a network whose DNS registers DHCP names, that is the
  name the board answers to there. The serial monitor shows it.

  OTA updates need a one-time setup per board: see README.md in the WaveformGenerator
  example. Giga OTA storage is included here and needs the Arduino_Portenta_OTA library.
  The password is "password".
*/

#pragma once

#ifndef HOST_NAME
#define HOST_NAME "BlaeckTCP"
#endif

// The fixed address, for when no DHCP server answers.
#define NETWORK_FALLBACK_IP 192, 168, 10, 177
#define NETWORK_FALLBACK_GATEWAY 192, 168, 10, 1
#define NETWORK_FALLBACK_SUBNET 255, 255, 0, 0

inline void networkWaitForSerial()
{
  const unsigned long started = millis();
  while (!Serial && millis() - started < 3000UL)
    delay(10);
}

// The server line, with the name Bonjour answers to when it runs.
inline void networkPrintServer(const Printable &ip, uint16_t port)
{
  Serial.print(F("BlaeckTCP server: "));
#if defined(NETWORK_WITH_SERVICES)
  Serial.print(F(HOST_NAME));
  Serial.print(':');
  Serial.print(port);
  Serial.print(F(" ("));
#endif
  Serial.print(ip);
  Serial.print(':');
  Serial.print(port);
#if defined(NETWORK_WITH_SERVICES)
  Serial.print(')');
#endif
  Serial.println();
}

// ---------------------------------------------------------------------------------------
#if defined(ARDUINO_ESP32_POE) || defined(ARDUINO_WT32_ETH01)
// ---------------------------------------------------------------------------------------

// The PHY pins come from the board definition, which is why the board has to be selected
// as the ESP32-PoE or the WT32-ETH01 rather than as a generic ESP32.
#include <ETH.h>

#if defined(NETWORK_WITH_SERVICES)
// The upload listener runs on the core's WiFi server, which serves every interface,
// Ethernet included.
#include <WiFi.h>
#include <ESPmDNS.h>
// Must be the Library Manager's ArduinoOTA, not the one bundled with the ESP32 core; see
// the README. ESPmDNS announces the upload service, so ArduinoOTA doesn't.
#define NO_OTA_PORT
#include <ArduinoOTA.h>
#endif

// Also reports the link going up and down while the sketch runs.
inline void networkEvent(arduino_event_id_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    // Before DHCP asks, so the DHCP server learns the name too.
    ETH.setHostname(HOST_NAME);
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    Serial.println(F("Ethernet connected."));
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    Serial.println(F("Ethernet cable is not connected."));
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    Serial.print(F("MAC: "));
    Serial.print(ETH.macAddress());
    Serial.print(F(", IPv4: "));
    Serial.print(ETH.localIP());
    Serial.print(F(", "));
    Serial.print(ETH.subnetMask());
    Serial.print(F(", "));
    Serial.println(ETH.gatewayIP());
    break;
  default:
    break;
  }
}

inline void networkBegin(uint16_t port)
{
  networkWaitForSerial();
  Serial.println(F("Looking for an address..."));
  Network.onEvent(networkEvent);
  ETH.begin();

  unsigned long waitUntil = millis() + 30000;
  while (!ETH.hasIP() && millis() < waitUntil)
    delay(50);
  if (!ETH.hasIP())
  {
    IPAddress gateway(NETWORK_FALLBACK_GATEWAY);
    ETH.config(IPAddress(NETWORK_FALLBACK_IP), gateway, IPAddress(NETWORK_FALLBACK_SUBNET), gateway);
  }

#if defined(NETWORK_WITH_SERVICES)
  MDNS.begin(HOST_NAME);
  MDNS.setInstanceName(HOST_NAME);
  MDNS.enableArduino(65280, true);
  MDNS.addService("blaeck", "tcp", port);
  ArduinoOTA.begin(ETH.localIP(), HOST_NAME, "password", InternalStorage);
#endif

  networkPrintServer(ETH.localIP(), port);
}

inline void networkLoop()
{
#if defined(NETWORK_WITH_SERVICES)
  ArduinoOTA.poll();
#endif
}

// ---------------------------------------------------------------------------------------
#elif defined(ARDUINO_AVR_MEGA2560) || defined(ARDUINO_GIGA)
// ---------------------------------------------------------------------------------------

#include <SPI.h>
#include <Ethernet.h>

#if defined(NETWORK_WITH_SERVICES)
#include <EthernetBonjour.h>
// EthernetBonjour announces the upload service, so ArduinoOTA doesn't.
#define NO_OTA_PORT
#include <ArduinoOTA.h>
// Where a received sketch waits until the board restarts into it: a file on the QSPI flash
// on the Giga, the second half of the program flash on the Mega.
#if defined(ARDUINO_GIGA)
#include <Arduino_Portenta_OTA.h>
#include <BlockDevice.h>
#include <MBRBlockDevice.h>
#include <FATFileSystem.h>
#include "OTAStorage.h"

// The bootloader applies UPDATE.BIN from QSPI partition 2 after a restart.
// Mount it directly: Arduino_Portenta_OTA::begin() also opens WiFi certificates,
// which are unnecessary for an update pushed over Ethernet.
class QspiOtaStorageClass : public ExternalOTAStorage {

public:

  virtual int open(int length) {
    (void)length;

    if (!Arduino_Portenta_OTA::isOtaCapable()) {
      Serial.println("This board's bootloader cannot apply an update from QSPI.");
      return -1;
    }

    // Mounted once and left mounted: the bootloader is told about the file after it is
    // written, and unmounting in between would only be a chance to fail.
    if (!_mounted) {
      int err = _fs.mount(&_partition);
      if (err) {
        Serial.print("The OTA partition would not mount, error ");
        Serial.println(err);
        Serial.println("Run QSPIFormat once, from the core's STM32H747_System examples.");
        return -2;
      }
      _mounted = true;
    }

    _file = fopen("/fs/UPDATE.BIN", "wb");
    if (!_file) {
      Serial.println("UPDATE.BIN could not be opened for writing.");
      return -3;
    }

    return 1;
  }

  virtual size_t write(uint8_t c) {
    if (fwrite(&c, 1, 1, _file) != 1) {
      Serial.println("Writing the update to QSPI failed.");
      fclose(_file);
      _file = nullptr;
      return 0;
    }
    return 1;
  }

  virtual void close() {
    if (_file) {
      fclose(_file);
      _file = nullptr;
    }
  }

  // What is left behind when an upload does not finish. A half-written UPDATE.BIN would be
  // applied on the next start as though it were whole.
  virtual void clear() {
    remove("/fs/UPDATE.BIN");
  }

  virtual void apply() {
    // Records in the backup registers where the update is and how long it is, then restarts.
    // The bootloader reads those before anything else runs.
    Arduino_Portenta_OTA_QSPI ota(QSPI_FLASH_FATFS_MBR, 2);

    if (ota.update() != Arduino_Portenta_OTA::Error::None) {
      Serial.println("The update was written but the board would not be told about it.");
      return;
    }

    ota.reset();
  }

  // The partition, less a little room: an update is a file on a filesystem rather than a
  // region, so what fits is what the filesystem leaves.
  virtual long maxSize() {
    return 5 * 1024 * 1024 - 64 * 1024;
  }

private:

  mbed::MBRBlockDevice _partition{mbed::BlockDevice::get_default_instance(), 2};
  mbed::FATFileSystem _fs{"fs"};
  bool _mounted = false;
  FILE* _file = nullptr;
};

QspiOtaStorageClass QspiStorage;
OTAStorage &OtaStorage = QspiStorage;
#else
OTAStorage &OtaStorage = InternalStorage;
#endif
#endif

// The shield's MAC address. Give each board on the same network its own.
#ifndef NETWORK_MAC
#define NETWORK_MAC "DE:AD:BE:EF:FE:ED"
#endif

byte networkMac[6];

// Only an address from DHCP has a lease to renew.
bool networkLeased = false;

// Reads NETWORK_MAC, written as DE:AD:BE:EF:FE:ED or DE-AD-BE-EF-FE-ED.
inline bool networkReadMac(const char *text)
{
  for (byte i = 0; i < 6; i++)
  {
    byte value = 0;
    for (byte j = 0; j < 2; j++)
    {
      char c = *text++;
      if (c >= '0' && c <= '9')
        value = value * 16 + (c - '0');
      else if (c >= 'A' && c <= 'F')
        value = value * 16 + (c - 'A' + 10);
      else if (c >= 'a' && c <= 'f')
        value = value * 16 + (c - 'a' + 10);
      else
        return false;
    }
    networkMac[i] = value;

    char separator = *text++;
    if (i < 5 ? separator != ':' && separator != '-' : separator != 0)
      return false;
  }
  return true;
}

inline void networkBegin(uint16_t port)
{
  networkWaitForSerial();
  if (!networkReadMac(NETWORK_MAC))
  {
    Serial.println(F("NETWORK_MAC is not a MAC address. Write it as DE:AD:BE:EF:FE:ED."));
    while (true)
      delay(1);
  }

  Serial.println(F("Looking for an address..."));
  networkLeased = Ethernet.begin(networkMac, 30000, 2000) != 0;
  if (!networkLeased)
  {
    IPAddress gateway(NETWORK_FALLBACK_GATEWAY);
    Ethernet.begin(networkMac, IPAddress(NETWORK_FALLBACK_IP), gateway, gateway, IPAddress(NETWORK_FALLBACK_SUBNET));
  }

  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println(F("No Ethernet shield found."));
    while (true)
      delay(1);
  }

  // begin() returns before the link is up, so wait for it briefly.
  unsigned long settle = millis() + 2000;
  while (Ethernet.linkStatus() == LinkOFF && millis() < settle)
    delay(50);
  if (Ethernet.linkStatus() == LinkOFF)
    Serial.println(F("Ethernet cable is not connected."));

#if defined(NETWORK_WITH_SERVICES)
  EthernetBonjour.begin(HOST_NAME);
  EthernetBonjour.addServiceRecord(HOST_NAME "._arduino", 65280, MDNSServiceTCP,
                                   "\x0d" "ssh_upload=no"
                                   "\x0c" "tcp_check=no"
                                   "\x0f" "auth_upload=yes"
                                   "\x0d" "board=arduino");
  EthernetBonjour.addServiceRecord(HOST_NAME "._blaeck", port, MDNSServiceTCP);
  ArduinoOTA.begin(Ethernet.localIP(), HOST_NAME, "password", OtaStorage);
#endif

  networkPrintServer(Ethernet.localIP(), port);
  Serial.print(F("MAC: "));
  Serial.println(F(NETWORK_MAC));

  // What the Ethernet library sent with its DHCP request. Nothing is sent on the fallback
  // address.
  if (networkLeased)
  {
    Serial.print(F("DHCP host name: WIZnet"));
    for (byte i = 3; i < 6; i++)
    {
      if (networkMac[i] < 0x10)
        Serial.print('0');
      Serial.print(networkMac[i], HEX);
    }
    Serial.println();
  }
}

inline void networkLoop()
{
#if defined(NETWORK_WITH_SERVICES)
  ArduinoOTA.poll();
  EthernetBonjour.run();
#endif
  if (networkLeased)
    Ethernet.maintain();
}

// ---------------------------------------------------------------------------------------
#else
#error "NetworkSetup.h covers the Mega, the Giga, the ESP32-PoE and the WT32-ETH01. For another board, see the examples under more, or the Ethernet library's."
#endif
