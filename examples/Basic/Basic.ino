/*
  Basic.ino

  The smallest useful sketch: two numbers, registered as signals, sent out on
  the interval the host asks for.

  A signal is a value that gets sampled and logged. Registering one is all it
  takes - tick() reads it and writes the frame, so nothing here has to know
  when a transmission is due.

  Runs on the boards NetworkSetup.h covers: the Mega and the Giga with an Ethernet
  shield, the ESP32-PoE and the WT32-ETH01. Select the two ESP32 boards under their own
  names, OLIMEX ESP32-POE and WT32-ETH01 Ethernet Module, not as ESP32 Dev Module. For
  other boards, see the examples under more.

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

  Author: Sebastian Strobl,
  More information on: https://github.com/sebaJoSt/BlaeckTCP
*/

#include "Arduino.h"
#define HOST_NAME "Basic"
#include "NetworkSetup.h"
#include "BlaeckTCP.h"

#define ExampleVersion "1.0"

// Instantiate a new BlaeckTCP object
BlaeckTCP Blaeck;

// The port hosts and terminals connect to.
#define SERVER_PORT 23

// Signals
float randomSmallNumber;
long randomBigNumber;

void setup()
{
  // Initialize Serial port
  Serial.begin(115200);

  // Gets the board online; see NetworkSetup.h.
  networkBegin(SERVER_PORT);

  // Setup BlaeckTCP. The library reports on the terminal connections.
  Blaeck.begin(SERVER_PORT)
      .withSignals(2)
      .withDebugStream(&Blaeck.Terminal);

  // Names the device wherever it turns up
  Blaeck.DeviceName = HOST_NAME;
  Blaeck.DeviceFWVersion = ExampleVersion;

  // F() keeps the name in flash instead of SRAM, which is worth having on a
  // Mega and costs nothing anywhere else.
  Blaeck.addSignal(F("Small Number"), &randomSmallNumber);
  Blaeck.addSignal(F("Big Number"), &randomBigNumber);
}

void loop()
{
  UpdateRandomNumbers();

  // Reads what has come in and writes the signals when the interval is up.
  Blaeck.tick();

  // Keeps the network running: the DHCP lease, and OTA and Bonjour where they are on.
  networkLoop();
}

void UpdateRandomNumbers()
{
  // Random small number from 0.00 to 10.00
  randomSmallNumber = random(1001) / 100.0;

  // Random big number from 2 000 000 000 to 2 100 000 000
  randomBigNumber = random(2000000000, 2100000001);
}
