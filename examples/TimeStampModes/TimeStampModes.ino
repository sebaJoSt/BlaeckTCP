/*
  TimeStampModes.ino

  Every data frame can carry a time, and this sketch shows the three modes it
  can be sent in. Pick one with TIMESTAMP_MODE below, then flash the board.

    BLAECK_NO_TIMESTAMP  no time is sent, so the host timestamps on arrival
    BLAECK_MICROS        micros() since power-up, supplied by the library
    BLAECK_UNIX          the wall clock, the only mode needing a clock source

  A networked board can take the wall clock from the network. On the ESP32-PoE and
  the WT32-ETH01 this sketch asks an NTP server, which the ESP32 core does for you.
  An RTC is the other common source, and BlaeckSerial's TimeStampModes example shows
  that with the one built into the Arduino UNO R4. On the Mega and the Giga, this
  sketch runs the first two modes only.

  Author: Sebastian Strobl,
  More information on: https://github.com/sebaJoSt/BlaeckTCP
*/

#include "Arduino.h"
#define HOST_NAME "TimeStampModes"
#include "NetworkSetup.h"
#include "BlaeckTCP.h"

#define ExampleVersion "1.0"

// The mode to run. This is the only line to change when trying another one.
#define TIMESTAMP_MODE BLAECK_MICROS

// The ESP32 core keeps a clock set from NTP; the other boards here have none.
#if defined(ESP32)
#include <sys/time.h>
#define HAS_NETWORK_CLOCK 1
#else
#define HAS_NETWORK_CLOCK 0
#endif

static_assert(TIMESTAMP_MODE != BLAECK_UNIX || HAS_NETWORK_CLOCK,
              "BLAECK_UNIX needs a clock. This example takes it from NTP on an ESP32; "
              "on this board, add an RTC or an NTP client.");

// Instantiate a new BlaeckTCP object
BlaeckTCP Blaeck;

// The port hosts and terminals connect to.
#define SERVER_PORT 23

// Signals
float sine;

void setup()
{
  // Initialize Serial port
  Serial.begin(115200);

  // Gets the board online; see NetworkSetup.h.
  networkBegin(SERVER_PORT);

#if HAS_NETWORK_CLOCK
  // UTC from the network. The clock reads 1970 until the first answer arrives,
  // usually within a few seconds.
  if (TIMESTAMP_MODE == BLAECK_UNIX)
    configTime(0, 0, "pool.ntp.org");
#endif

  // Setup BlaeckTCP, with room for one signal
  Blaeck.begin(SERVER_PORT).withSignals(1);

  Blaeck.DeviceName = "Timestamp Modes";
  Blaeck.DeviceHWVersion = NETWORK_BOARD;
  Blaeck.DeviceFWVersion = ExampleVersion;

  Blaeck.addSignal(F("Sine_1"), &sine);

  Blaeck.setTimestampMode(TIMESTAMP_MODE);

#if HAS_NETWORK_CLOCK
  // Only BLAECK_UNIX needs a clock source. The other two modes bring their own.
  if (TIMESTAMP_MODE == BLAECK_UNIX)
    Blaeck.setTimestampCallback(GetNtpUnixTimeMicros);
#endif
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

#if HAS_NETWORK_CLOCK
unsigned long long GetNtpUnixTimeMicros()
{
  struct timeval now;
  gettimeofday(&now, nullptr);
  return (unsigned long long)now.tv_sec * 1000000ULL + now.tv_usec;
}
#endif
