/*
  TimestampsNTP.ino

  Timestamp data with the network clock using BLAECK_UNIX and a callback that returns
  microseconds since the Unix epoch. Runs on the ESP32-PoE and WT32-ETH01 through
  NetworkSetup.h and needs access to an NTP server.

  The TCP server starts only after the first successful clock sync, so readings cannot
  be logged with a timestamp in 1970. If NTP is unreachable, the serial monitor reports
  it and the sketch keeps waiting.

  For timestamps without a network clock, see docs/network.md. For an RTC instead, see
  BlaeckSerial's more/TimestampsRTC example on the Arduino UNO R4.

  Author: Sebastian Strobl,
  More information on: https://github.com/sebaJoSt/BlaeckTCP
*/

#include "Arduino.h"
#define HOST_NAME "TimestampsNTP"
#include "NetworkSetup.h"
#include "BlaeckTCP.h"

#define ExampleVersion "1.0"

#if !defined(ARDUINO_ARCH_ESP32)
#error "TimestampsNTP needs an ESP32 network clock. See docs/network.md for timestamp modes without a clock source."
#endif
#include <sys/time.h>
#include <time.h>

// Instantiate a new BlaeckTCP object
BlaeckTCP Blaeck;

// The port hosts and terminals connect to.
#define SERVER_PORT 23

// Signals
float sine;

unsigned long long GetNtpUnixTimeMicros()
{
  struct timeval now;
  gettimeofday(&now, nullptr);
  return (unsigned long long)now.tv_sec * 1000000ULL + now.tv_usec;
}

void setup()
{
  // Initialize Serial port
  Serial.begin(115200);

  // Gets the board online; see NetworkSetup.h.
  networkBegin(SERVER_PORT);

  configTime(0, 0, "pool.ntp.org");
  Serial.println(F("Waiting for the network clock..."));
  struct tm clock;
  while (!getLocalTime(&clock, 10000))
  {
    Serial.println(F("NTP has not synchronized. Check network access to pool.ntp.org; retrying."));
    networkLoop();
  }

  // Setup BlaeckTCP, with room for one signal
  Blaeck.begin(SERVER_PORT).withSignals(1);

  Blaeck.DeviceName = HOST_NAME;
  Blaeck.DeviceFWVersion = ExampleVersion;

  Blaeck.addSignal(F("Sine_1"), &sine);

  Blaeck.setTimestampCallback(GetNtpUnixTimeMicros);
  Blaeck.setTimestampMode(BLAECK_UNIX);
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
  sine = sin(millis() * 0.00005);
}
