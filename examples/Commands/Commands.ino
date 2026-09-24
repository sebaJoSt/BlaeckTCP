/*
  Commands.ino

  How to add your own commands. There are two kinds:

    Plain   onCommand()          You parse the parameters yourself.
                                 No automatic dashboard control.

    Typed   onSwitchCommand()    You declare what the command is. The library
            onButtonCommand()    checks the value first, and describes the
                                 command well enough that Loggbok can publish
                                 it and Home Assistant can show a control.

  <SwitchLED> and <LED> switch the same LED, one plain and one typed, so the
  difference is easy to see.

  A typed switch also names a state signal ("LED_State"). That is what the
  control follows, so it shows what the board really did instead of assuming
  the command worked.

  Author: Sebastian Strobl,
  More information on: https://github.com/sebaJoSt/BlaeckTCP

  Connect a telnet client such as PuTTY to the address the serial monitor prints,
  port 23, and type the commands below. The replies appear there too.

  Command syntax:

    <COMMAND,PARAMETER01,PARAMETER02,...,PARAMETER10>

    Parameters arrive as text. Plain handlers must check the parameter count and the whole
    value, not just a numeric prefix. An empty one keeps its slot, so <SwitchLED,> below
    reads as params[0][0] == '\0'.

  The circuit:
    - No wiring required, the on-board LED is used.
      LED_BUILTIN is pin 13 on the MEGA, and the right pin on the Giga.
    - The ESP32-PoE and the WT32-ETH01 have no LED of their own: wire one, with
      a resistor, to the pin set below.

  Typed, and so also controls in Home Assistant:

        <LED,1>                       Turn on the LED
        <LED,0>                       Turn off the LED
        <LED,7>                       Rejected: a switch only accepts 0 or 1
        <Ping>                        Takes no value. Answers on the "Status"
                                      state channel with how long the board
                                      has been running.

  Plain commands, callable by a host or terminal but not auto-discovered as controls:

        <SwitchLED,1>                 Turn on the LED
        <SwitchLED,0>                 Turn off the LED
        <SwitchLED,ON>                Also accepts text: a plain command
        <SwitchLED,OFF>               parses its value itself
        <SwitchLED,>                  Empty parameter -> uses default (OFF)
        <SwitchLED,garbage>           Reports an error; leaves the LED unchanged
        <Print,Hello,3>               Prints Hello three times; count must be 1..10
        <Print,Hello,3abc>            Reports an error; prints no copies of Hello
        <Print,Hello,11>              Reports an error; does not clamp the count

  Plain-handler errors are terminal text, not protocol rejection acknowledgements.
  A protocol host does not receive Blaeck.Terminal text; keep a terminal connected to see it.
*/

#include "Arduino.h"
#define HOST_NAME "Commands"
#include "NetworkSetup.h"
#include "BlaeckTCP.h"
#include <stdlib.h>

#define ExampleVersion "1.0"

// Instantiate a new BlaeckTCP object
BlaeckTCP Blaeck;

// The port hosts and terminals connect to.
#define SERVER_PORT 23

// Sets the pin number:
#ifdef LED_BUILTIN
const int ledPin = LED_BUILTIN;
#else
// No on-board LED: set this to the pin you wired one to.
const int ledPin = 4;
#endif

// Mirrors the LED. Registered as a signal so <LED> can point at it.
bool ledState = false;

void onSwitchLED(const char *command, const char *const *params, byte paramCount);
void onLED(const char *command, const char *const *params, byte paramCount);
void onPing(const char *command, const char *const *params, byte paramCount);
void onPrint(const char *command, const char *const *params, byte paramCount);
void setLed(bool on);

void setup()
{
  // Set the digital pin as output:
  pinMode(ledPin, OUTPUT);

  // Initialize Serial port
  Serial.begin(115200);

  // Gets the board online; see NetworkSetup.h.
  networkBegin(SERVER_PORT);

  // Setup BlaeckTCP, room for one signal. The library's messages go to the terminal too.
  Blaeck.begin(SERVER_PORT).withSignals(1).withDebugStream(&Blaeck.Terminal);

  // Names the device wherever it turns up
  Blaeck.DeviceName = HOST_NAME;
  Blaeck.DeviceFWVersion = ExampleVersion;

  // The state signal the typed switch below refers to
  Blaeck.addSignal(F("LED_State"), &ledState);

  // Plain: listed by name only, so it can be sent but not turned into a control.
  Blaeck.onCommand("SwitchLED", onSwitchLED);
  Blaeck.onCommand("Print", onPrint);

  // Typed: checked by the library and described well enough for a control.
  // A switch is 0/1 and points at a state signal; a button carries no value.
  Blaeck.onSwitchCommand("LED", onLED).withStateFromSignal(F("LED_State"));
  Blaeck.onButtonCommand("Ping", onPing);

  // Where <Ping> answers. Declared here so the sensor exists from the start.
  Blaeck.addStateChannel(F("Status"), BlaeckText).withIcon(F("mdi:message-text"));
}

void loop()
{
  // Handles incoming commands, and writes the signals on the interval.
  Blaeck.tick();

  // Keeps the network running: the DHCP lease, and OTA and Bonjour where they are on.
  networkLoop();
}

// Plain command: the parameters arrive as text and you decide what they mean.
void onSwitchLED(const char *command, const char *const *params, byte paramCount)
{
  (void)command;
  if (paramCount != 1)
  {
    Blaeck.Terminal.println(F("SwitchLED expects exactly one value: 0, 1, ON or OFF."));
    return;
  }
  // <SwitchLED,> sends an empty field
  if (params[0][0] == '\0')
  {
    Blaeck.Terminal.println("No state given, using default (OFF).");
    setLed(false);
    return;
  }
  // Parsing it yourself means accepting whatever spelling suits you.
  // equalsFlash() compares against a name kept in flash instead of SRAM.
  if (Blaeck.equalsFlash(params[0], F("ON")) || Blaeck.equalsFlash(params[0], F("1")))
  {
    setLed(true);
    Blaeck.Terminal.println("LED is ON.");
    return;
  }
  if (Blaeck.equalsFlash(params[0], F("OFF")) || Blaeck.equalsFlash(params[0], F("0")))
  {
    setLed(false);
    Blaeck.Terminal.println("LED is OFF.");
    return;
  }

  Blaeck.Terminal.println(F("Invalid SwitchLED value. Use 0, 1, ON or OFF; LED unchanged."));
}

// Typed switch: the library rejects <LED,7> before this runs, so the value
// here is always 0 or 1.
void onLED(const char *command, const char *const *params, byte paramCount)
{
  (void)command;
  if (paramCount < 1 || params[0][0] == '\0')
  {
    return;
  }
  setLed(atoi(params[0]) == 1);
  Blaeck.Terminal.println(ledState ? "LED is ON." : "LED is OFF.");
}

/* Typed button: no value to parse.

   A button has no state signal, so it answers on the "Status" state channel
   instead. That is a frame and reaches Home Assistant; Blaeck.Terminal would
   only reach a terminal.
*/
void onPing(const char *command, const char *const *params, byte paramCount)
{
  (void)command;
  (void)params;
  (void)paramCount;
  // %lu is fine on AVR; only float formatting (%f) is left out of printf there.
  char text[40];
  unsigned long seconds = millis() / 1000UL;
  snprintf(text, sizeof(text), "alive, running for %lu s", seconds);
  Blaeck.writeState(F("Status"), text);
}

/* Exemplary command using two parameters:
   Example: <Print,Hello,3>
*/
void onPrint(const char *command, const char *const *params, byte paramCount)
{
  (void)command;
  if (paramCount != 2)
  {
    Blaeck.Terminal.println(F("Print expects exactly two parameters: text and count."));
    return;
  }
  // Digits only, with no ignored suffix. The bound keeps this handler short.
  char *end;
  const long repeats = strtol(params[1], &end, 10);
  if (params[1][0] < '0' || params[1][0] > '9' || *end != '\0' ||
      repeats < 1 || repeats > 10)
  {
    Blaeck.Terminal.println(F("Print count must be decimal digits with a value from 1 to 10."));
    return;
  }
  for (int i = 0; i < repeats; i++)
  {
    Blaeck.Terminal.println(params[0]);
  }
}

// Keeps the pin and the signal in step, whichever command was used.
void setLed(bool on)
{
  ledState = on;
  digitalWrite(ledPin, on ? HIGH : LOW);
}
