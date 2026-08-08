/*
  CommandsEthernet.ino

  This is a sample sketch to show how to use BlaeckTCP to
  implement your own commands.

  It registers two kinds of command, on purpose:

    Plain    onCommand(...)         You parse the parameters yourself, and
                                    nothing is declared about the value.

    Typed    onSwitchCommand(...)   You declare what the command is. The
             onButtonCommand(...)   library validates the value before your
                                    handler runs, and describes the command
                                    so a host (e.g. Loggbok / Home Assistant)
                                    can create an entity for it by itself.

  Every registered command is listed in <BLAECK.WRITE_COMMANDS>, plain ones
  included, so a host can offer a complete command palette. What differs is
  the metadata: a plain entry says only "this command exists", while a typed
  entry carries its kind, its allowed values and its state signal - which is
  what a dashboard needs to build a control for it.

  <SwitchLED> and <LED> both switch the same on-board LED, one plain and one
  typed, so you can compare the two entries side by side in that list.

  How a command reports back differs accordingly:

    <SwitchLED>   CommandingClient.println(...)  Reaches the Telnet client that
                                                 sent the command, and only it.
                                                 A Blaeck host skips anything
                                                 that is not a frame.
    <LED>         its state signal               The dashboard follows LED_State.
    <Ping>        writeMessage(...)              A button has no state signal, so
                                                 it pushes a line to a named
                                                 message channel, broadcast to
                                                 every connected client.

  The command syntax for implementing your own commands:

    Command:         <COMMAND,PARAMETER01,PARAMETER02,...,PARAMETER10>
                     <-  full payload size is architecture-dependent ->
                     AVR: up to 48 chars, non-AVR: up to 96 chars
                     <-         --  max. 10 parameters ---          ->

    COMMAND:         String token (handler key used in onCommand)
    PARAMETER01..10  String tokens (convert with atoi/atol/atof as needed)
    Start Marker*:    <
    End Marker*:      >
    Separation*:      ,

      * Not allowed in COMMAND or parameter tokens

    Empty parameters are preserved positionally and default to empty string / 0,
    e.g. <COMMAND,,PARAMETER02>      <- PARAMETER01 is empty, PARAMETER02 stays in its slot
    To check if a parameter was provided: params[i][0] == '\0' means empty

  Circuit:
    - Ethernet shield attached to pins 10, 11, 12, 13
    - Use the on-board LED
      Note: Most Arduinos have an on-board LED you can control. On the UNO and MEGA
            it is attached to digital pin 13. LED_BUILTIN is set to the correct LED pin
            independent of which board is used.

  Usage:
    - Upload the sketch to your Arduino.
    - Open a Telnet Client (e.g. PuTTY) and connect to IP Address 192.168.1.177 (Port 23)
    - Type the following commands and press enter:

        Your own commands:
        <SwitchLED,1>                 Turn on the LED   (plain)
        <SwitchLED,0>                 Turn off the LED  (plain)
        <SwitchLED,>                  Empty param → uses default (OFF)
        <LED,1>                       Turn on the LED   (typed switch)
        <LED,0>                       Turn off the LED  (typed switch)
        <LED,7>                       Rejected: a switch only accepts 0 or 1
        <Ping>                        Typed button, takes no value. The board
                                      answers on the "Status" message channel
                                      with how long it has been running, so the
                                      reply reaches a host and not just a
                                      Telnet client.
        <Print,Bye Bye,1>             String parameters

        Built-in Blaeck commands:
        <BLAECK.GET_DEVICES>          Writes the device's information to the PC
        <BLAECK.WRITE_SYMBOLS>        Writes the symbol list to the PC
        <BLAECK.WRITE_COMMANDS>       Writes the command list to the PC
                                      All four commands appear here. The typed
                                      ones carry their kind and metadata, the
                                      plain ones only their name.
        <BLAECK.WRITE_DATA>           Writes the data to the PC

  created by Sebastian Strobl
  More information on: https://github.com/sebaJoSt/BlaeckTCP
 */

#include <SPI.h>
#include <Ethernet.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_CLIENTS 8

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

// Sets the pin number:
const int ledPin = LED_BUILTIN;

// Mirrors the LED state. Registered as a signal so the typed <LED> command
// can point at it, which lets a dashboard show the state it is controlling.
bool ledState = false;

void onSwitchLED(const char *command, const char *const *params, byte paramCount);
void onLED(const char *command, const char *const *params, byte paramCount);
void onPing(const char *command, const char *const *params, byte paramCount);
void onPrint(const char *command, const char *const *params, byte paramCount);
void setLed(bool on);

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
  // Set the digital pin as output:
  pinMode(ledPin, OUTPUT);

  // You can use Ethernet.init(pin) to configure the CS pin
  // Ethernet.init(10);  // Most Arduino shields
  // Ethernet.init(5);   // MKR ETH Shield
  // Ethernet.init(0);   // Teensy 2.0
  // Ethernet.init(20);  // Teensy++ 2.0
  // Ethernet.init(15);  // ESP8266 with Adafruit FeatherWing Ethernet
  // Ethernet.init(33);  // ESP32 with Adafruit FeatherWing Ethernet

  // initialize the Ethernet device
  Ethernet.begin(mac, ip, myDns, gateway, subnet);

  // Open serial communications (used for debug output only)
  Serial.begin(115200);

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
      1,           // Maximal signal count used;
      SERVER_PORT  // TCP server port
  );

  // Reported by <BLAECK.GET_DEVICES>, and used by a host to name the device
  BlaeckTCP.DeviceName = "Command Demo Ethernet";
  BlaeckTCP.DeviceHWVersion = "Arduino Mega 2560 Rev3";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  // The state signal the typed switch below refers to
  BlaeckTCP.addSignal("LED_State", &ledState);

  // Plain: you parse the parameters yourself. Listed by name only, so a host
  // knows the command exists but cannot build a control for it.
  BlaeckTCP.onCommand("SwitchLED", onSwitchLED);
  BlaeckTCP.onCommand("Print", onPrint);

  // Typed: validated by the library, and listed with the metadata a host needs
  // to create an entity. A switch is 0/1 and mirrors a state signal; a button
  // carries no value.
  BlaeckTCP.onSwitchCommand("LED", onLED, F("LED_State"));
  BlaeckTCP.onButtonCommand("Ping", onPing);
}

void loop()
{
  /* Keeps watching for TCP input and dispatches registered handlers when
     input with the correct syntax is detected. tick() also writes the
     signals in a user-set interval; use BlaeckTCP.read() instead if you
     only want commands and no data.
  */
  BlaeckTCP.tick();
}

/* Plain command. You get the raw parameters and decide what they mean,
   including what an empty one should do. CommandingClient is the client that
   sent this command, so the reply goes to it alone.
*/
void onSwitchLED(const char *command, const char *const *params, byte paramCount)
{
  if (paramCount < 1)
  {
    return;
  }
  // Detect empty parameter: <SwitchLED,> sends an empty field
  if (params[0][0] == '\0')
  {
    BlaeckTCP.CommandingClient.println("No state given, using default (OFF).");
    setLed(false);
    return;
  }
  int state = atoi(params[0]);
  if (state == 1)
  {
    setLed(true);
    BlaeckTCP.CommandingClient.println("LED is ON.");
    return;
  }
  if (state == 0)
  {
    setLed(false);
    BlaeckTCP.CommandingClient.println("LED is OFF.");
    return;
  }
}

/* Typed switch. The library has already checked that the value is 0 or 1
   before this runs - <LED,7> is rejected and never reaches the handler - so
   there is less to guard against here.
*/
void onLED(const char *command, const char *const *params, byte paramCount)
{
  (void)command;
  if (paramCount < 1 || params[0][0] == '\0')
  {
    return;
  }
  setLed(atoi(params[0]) == 1);
  BlaeckTCP.CommandingClient.println(ledState ? "LED is ON." : "LED is OFF.");
}

/* Typed button. Carries no value, so there is nothing to parse.

   A button has no state signal, so writeMessage() is how it reports back: it
   pushes a line to a named 0x90 message channel, broadcast to every connected
   client, which a host can surface as a text sensor. CommandingClient.println()
   would reach only the Telnet client that asked - a Blaeck host skips anything
   that is not a frame.
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
  BlaeckTCP.writeMessage("Status", text);
}

/* Exemplary command using string parameters:
   Example: <Print,Bye Bye,1>
*/
void onPrint(const char *command, const char *const *params, byte paramCount)
{
  (void)command;
  if (paramCount < 2)
  {
    return;
  }

  int mode = atoi(params[1]);
  if (mode == 0)
  {
    BlaeckTCP.CommandingClient.println(params[0]);
    return;
  }
  if (mode == 1)
  {
    BlaeckTCP.CommandingClient.print(params[0]);
    BlaeckTCP.CommandingClient.println(" Miss American Pie");
    BlaeckTCP.CommandingClient.println("Drove my Chevy to the levee but the levee was dry");
    BlaeckTCP.CommandingClient.println("And them good ole boys were drinking whiskey and rye");
    BlaeckTCP.CommandingClient.println("Singin' this'll be the day that I die");
    BlaeckTCP.CommandingClient.println("This'll be the day that I die");
    return;
  }
}

// Keeps the pin and the state signal in step, so whichever command was used
// the dashboard sees the same value.
void setLed(bool on)
{
  ledState = on;
  digitalWrite(ledPin, on ? HIGH : LOW);
}
