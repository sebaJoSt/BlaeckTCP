/*
  WriteModes.ino

  Three signals, each sent a different way, so that when a value goes out is no
  longer decided by the interval alone.

    Sine_1  write() sends the value the moment it is set, without waiting for
            the interval. This is what event driven data needs: a limit switch
            or an alarm is worth little if it arrives a minute late.

    Sine_2  the variable is set directly, then markSignalUpdated() flags it.

    Sine_3  update() does both of those steps in one call.

  Sine_2 and Sine_3 leave the sending to tickUpdated(), which goes out on the
  interval like tick() does but carries only what actually changed. Where
  signals update slowly, that saves most of the traffic.

  Author: Sebastian Strobl,
  More information on: https://github.com/sebaJoSt/BlaeckTCP
*/

#include "Arduino.h"
#define HOST_NAME "WriteModes"
#include "NetworkSetup.h"
#include "BlaeckTCP.h"

#define ExampleVersion "1.0"

// Instantiate a new BlaeckTCP object
BlaeckTCP Blaeck;

// The port hosts and terminals connect to.
#define SERVER_PORT 23

// Signals
float sine_1;
float sine_2;
float sine_3;

// Each signal runs on its own schedule, so each gets its own timer.
struct Timer
{
  unsigned long interval;
  unsigned long lastRun;
  bool firstRun;

  bool isDue()
  {
    if (!firstRun && millis() - lastRun < interval)
      return false;

    firstRun = false;
    lastRun = millis();
    return true;
  }
};

Timer timer_1 = {100, 0, true};
Timer timer_2 = {2000, 0, true};
Timer timer_3 = {10000, 0, true};

void setup()
{
  // Initialize Serial port
  Serial.begin(115200);

  // Gets the board online; see NetworkSetup.h.
  networkBegin(SERVER_PORT);

  // Setup BlaeckTCP, with room for three signals
  Blaeck.begin(SERVER_PORT).withSignals(3);

  Blaeck.DeviceName = "Write Modes Demo";
  Blaeck.DeviceHWVersion = NETWORK_BOARD;
  Blaeck.DeviceFWVersion = ExampleVersion;

  // Add signals to BlaeckTCP
  Blaeck.addSignal(F("Sine_1"), &sine_1);
  Blaeck.addSignal(F("Sine_2"), &sine_2);
  Blaeck.addSignal(F("Sine_3"), &sine_3);
}

void loop()
{
  TransmitFirstSine();

  UpdateSecondSine();
  UpdateThirdSine();

  // Reads what has come in and sends the signals marked above.
  Blaeck.tickUpdated();

  // Keeps the network running: the DHCP lease, and OTA and Bonjour where they are on.
  networkLoop();
}

// Sent right here, every 100 ms, whatever the interval is set to.
void TransmitFirstSine()
{
  if (timer_1.isDue())
    Blaeck.write("Sine_1", sin(millis() * 0.00005));
}

// Set the variable yourself, then say it changed.
void UpdateSecondSine()
{
  if (timer_2.isDue())
  {
    sine_2 = sin(millis() * 0.00005);
    Blaeck.markSignalUpdated("Sine_2");
  }
}

// The same thing, in one call.
void UpdateThirdSine()
{
  if (timer_3.isDue())
    Blaeck.update("Sine_3", sin(millis() * 0.00005));
}
