/*
  WriteModesEthernet.ino

  This sketch demonstrates two different data transmission patterns:

  1. Direct Write (Sine_1):
     - Data is immediately transmitted after updating
     - Updates every 100ms using BlaeckSerial.write()

  2. Interval Mode (Sine_2 & Sine_3):
     - Data is marked as updated but transmitted only when tickUpdated() is called
     - Sine_2:
        - updates every 2 seconds
        - Data is calculated and stored in the signal variable
        - Signal is marked as updated using BlaeckSerial.markSignalUpdated()
     - Sine_3
        - updates every 10 seconds
        - Sames as Sine_2, but BlaeckSerial.update() combines updating and marking in a single function call

  created by Sebastian Strobl
  More information on: https://github.com/sebaJoSt/BlaeckTCP
*/

#include <SPI.h>
#include <Ethernet.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_CLIENTS 8
#define MAX_SIGNALS 10

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

// Signals
float sine_1;
float sine_2;
float sine_3;

unsigned long updateLastTimeDone_s1 = 0;
unsigned long updateInterval_s1 = 100; // 100ms interval
bool updateFirstTime_s1 = true;

unsigned long updateLastTimeDone_s2 = 0;
unsigned long updateInterval_s2 = 2000; // 2s interval
bool updateFirstTime_s2 = true;

unsigned long updateLastTimeDone_s3 = 0;
unsigned long updateInterval_s3 = 10000; // 10s interval
bool updateFirstTime_s3 = true;

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
      0b11111101   // Clients permitted to receive data messages; from right to left: client #0, #1, .. , #7
  );

  BlaeckTCP.DeviceName = "Basic Sine Number Generator";
  BlaeckTCP.DeviceHWVersion = "Arduino Mega 2560 Rev3";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  // Add signals to BlaeckTCP
  BlaeckTCP.addSignal(F("Sine_1"), &sine_1);
  BlaeckTCP.addSignal(F("Sine_2"), &sine_2);
  BlaeckTCP.addSignal(F("Sine_3"), &sine_3);

  // Start listening for clients
  TelnetPrint = NetServer(SERVER_PORT);
  TelnetPrint.begin();
}

void loop()
{
  TransmitFirstSine();

  UpdateSecondSine();
  UpdateThirdSine();

  BlaeckTCP.tickUpdated();
}

void TransmitFirstSine()
{
  if ((millis() - updateLastTimeDone_s1 >= updateInterval_s1) || updateFirstTime_s1)
  {
    updateLastTimeDone_s1 = millis();
    updateFirstTime_s1 = false;

    float calcSine = sin(millis() * 0.00005);
    BlaeckTCP.write("Sine_1", calcSine);
  }
}

void UpdateSecondSine()
{
  if ((millis() - updateLastTimeDone_s2 >= updateInterval_s2) || updateFirstTime_s2)
  {
    updateLastTimeDone_s2 = millis();
    updateFirstTime_s2 = false;

    sine_2 = sin(millis() * 0.00005);
    BlaeckTCP.markSignalUpdated("Sine_2");
  }
}

void UpdateThirdSine()
{
  if ((millis() - updateLastTimeDone_s3 >= updateInterval_s3) || updateFirstTime_s3)
  {
    updateLastTimeDone_s3 = millis();
    updateFirstTime_s3 = false;

    float calcSine = sin(millis() * 0.00005);
    BlaeckTCP.update("Sine_3", calcSine);
  }
}
