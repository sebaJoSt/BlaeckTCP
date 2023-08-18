/*
  StorageOfSignalNamesInFlashMemoryAVROnly.ino

  This example sketch is similar to SineGeneratorEthernet.ino but stores the
  Signal names in flash memory to save RAM.

  Storage of signal names in flash memory is currently only implemented in AVR architecture (ATMega328p,..).
  Other microcontrollers, like the ESP32, should have sufficient RAM.

  Circuit:
   Ethernet shield attached to pins 10, 11, 12, 13

 created by Sebastian Strobl
 */

#include <SPI.h>
#include <Ethernet.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define MAX_CLIENTS 8
#define SERVER_PORT 23
#define MAX_SIGNALS 5

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

// Signals
float sine;

// Define the signal names here in flash memory.
// Maximum length of each signal name is 50 chars
const char flashSignalName_1[] PROGMEM = "Sine_1";
const char flashSignalName_2[] PROGMEM = "Sine_2";
const char flashSignalName_3[] PROGMEM = "Sine_3";
const char flashSignalName_4[] PROGMEM = "Sine_4";
const char flashSignalName_5[] PROGMEM = "Sine_5";

// Add them to the table
// The later with BlaeckTCP::addSignal() added signals
// have to match with this table (same number and order)
PGM_P const flashSignalNameTable[] PROGMEM = {
    flashSignalName_1,
    flashSignalName_2,
    flashSignalName_3,
    flashSignalName_4,
    flashSignalName_5};

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
  BlaeckTCP.begin(
      MAX_CLIENTS, // Maximal number of allowed clients
      &Serial,     // Serial reference, used for debugging
      MAX_SIGNALS, // Maximal signal count used;
      0b11111101   // Clients allowed to receive Blaeck Data; from right to left: client #0, #1, .. , #7
  );

  BlaeckTCP.DeviceName = "Basic Sine Number Generator";
  BlaeckTCP.DeviceHWVersion = "Arduino Mega 2560 Rev3";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  // Add signals is still used, but here the passed signal name is empty ("")
  // Adding a signal with empty signal name enables lookup in flashSignalNameTable.
  // If signal name is NOT empty, normal RAM storage of variable is used
  for (int i = 1; i <= MAX_SIGNALS; i++)
  {
    BlaeckTCP.addSignal("", &sine);
  }

  // Passes the table to the library
  BlaeckTCP.setFlashSignalNameTable(flashSignalNameTable);

  // Start listening for clients
  TelnetPrint = NetServer(SERVER_PORT);
  TelnetPrint.begin();
}

void loop()
{
  UpdateSineNumbers();

  /*Keeps watching for commands from TCP client and
    transmits the data back to client at the user-set interval*/
  BlaeckTCP.tick();
}

void UpdateSineNumbers()
{
  sine = sin(millis() * 0.00005);
}