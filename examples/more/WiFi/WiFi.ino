/*
  WiFi.ino

  This is a sample sketch to show how to use the BlaeckTCP library to transmit data
  over WiFi to your PC (Client), at the interval a host asks for. It runs on the
  Arduino UNO R4 WiFi and on ESP32 boards.

  Setup:
    Enter your network's name and password in the arduino_secrets.h tab.

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

#if defined(ARDUINO_UNOWIFIR4)
#include <WiFiS3.h>
#else
#include <WiFi.h>
#endif
#include "arduino_secrets.h"
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define HOST_NAME "WiFi"
#define SERVER_PORT 23
#define MAX_CLIENTS 4

// Instantiate a new BlaeckTCP object
BlaeckTCP Blaeck;

// Signals
float randomSmallNumber;
long randomBigNumber;

void setup()
{
  // Open serial communications (used for debug output only)
  Serial.begin(115200);
  Serial.println();

#if defined(ARDUINO_UNOWIFIR4)
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with the WiFi module failed.");
    while (true)
      ;
  }
  if (WiFi.firmwareVersion() < WIFI_FIRMWARE_LATEST_VERSION)
    Serial.println("Please upgrade the WiFi firmware.");
#endif

  WiFi.setHostname(HOST_NAME);

#if defined(ARDUINO_UNOWIFIR4)
  // Keep trying: begin() waits for an answer, and gives up after a while.
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Connecting to ");
    Serial.println(SECRET_SSID);
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    delay(10000);
  }
#else
  // Ten seconds to connect, then start over.
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  Serial.print("Connecting to ");
  Serial.println(SECRET_SSID);
  for (int tries = 10; WiFi.status() != WL_CONNECTED; tries--)
  {
    if (tries == 0)
    {
      Serial.println("WiFi connect failed.");
      delay(1000);
      ESP.restart();
    }
    delay(1000);
  }
#endif
  Serial.println("WiFi connected.");

  Serial.print("BlaeckTCP Server: ");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.println(SERVER_PORT);

  // Setup BlaeckTCP. The library reports on the terminal connections.
  Blaeck.begin(SERVER_PORT)
      .withClients(MAX_CLIENTS)
      .withSignals(2)
      .withDebugStream(&Blaeck.Terminal);

  Blaeck.DeviceName = HOST_NAME;
  Blaeck.DeviceFWVersion = EXAMPLE_VERSION;

  // Add signals to BlaeckTCP
  Blaeck.addSignal(F("Small Number"), &randomSmallNumber);
  Blaeck.addSignal(F("Big Number"), &randomBigNumber);
}

void loop()
{
  UpdateRandomNumbers();

  // Handles connections and commands, and sends the signals when the interval is up.
  Blaeck.tick();
}

void UpdateRandomNumbers()
{
  // Random small number from 0.00 to 10.00
  randomSmallNumber = random(1001) / 100.0;

  // Random big number from 2 000 000 000 to 2 100 000 000
  randomBigNumber = random(2000000000, 2100000001);
}
