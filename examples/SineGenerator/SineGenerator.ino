/*
  SineGenerator.ino

  This is a sample sketch to show how to use the BlaeckTCP library to transmit five
  evenly phase-shifted sine waves from the Arduino board to Loggbok.
*/

#include "Arduino.h"
#define HOST_NAME "SineGenerator"
#include "NetworkSetup.h"
#include "BlaeckTCP.h"

#define ExampleVersion "1.0"

// Instantiate a new BlaeckTCP object
BlaeckTCP Blaeck;

// The port hosts and terminals connect to.
#define SERVER_PORT 23

// addSignal() keeps a pointer to these, so they have to be globals.
#define SIGNAL_COUNT 5
float sine[SIGNAL_COUNT];

void setup()
{
  // Initialize Serial port
  Serial.begin(115200);

  // Gets the board online; see NetworkSetup.h.
  networkBegin(SERVER_PORT);

  // Setup BlaeckTCP, with room for the signals
  Blaeck.begin(SERVER_PORT).withSignals(SIGNAL_COUNT);

  Blaeck.DeviceName = "Basic Sine Number Generator";
  Blaeck.DeviceHWVersion = NETWORK_BOARD;
  Blaeck.DeviceFWVersion = ExampleVersion;

  // The prefix stays in flash and the number is added when the name is sent, so
  // "Sine_1".."Sine_5" cost no SRAM at all.
  for (int i = 0; i < SIGNAL_COUNT; i++)
  {
    Blaeck.addSignal(F("Sine_"), &sine[i]).withNameSuffix(i + 1);
  }

  // Prints only if a signal was dropped, and names the call that would have made room.
  Blaeck.printRejections(&Serial);
}

void loop()
{
  UpdateSineNumbers();

  // Reads what has come in and writes the signals when the interval is up.
  Blaeck.tick();

  // Keeps the network running: the DHCP lease, and OTA and Bonjour where they are on.
  networkLoop();
}

void UpdateSineNumbers()
{
  float phase = millis() * 0.00005;

  for (int i = 0; i < SIGNAL_COUNT; i++)
    sine[i] = sin(phase + i * (TWO_PI / SIGNAL_COUNT));
}
