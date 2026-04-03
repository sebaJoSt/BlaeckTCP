/*
  SineGeneratorAndCommandsEthernet.ino

  This is a sample sketch combining SineGeneratorEthernet.ino and CommandsEthernet.ino

  Circuit:
    Ethernet shield attached to pins 10, 11, 12, 13

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
float sine;

// Sets the LED pin number
const int ledPin = LED_BUILTIN;

void onSwitchLED(const char *command, const char *const *params, byte paramCount);
void onSomeCommand(const char *command, const char *const *params, byte paramCount);
void onPrint(const char *command, const char *const *params, byte paramCount);

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
  for (int i = 1; i <= MAX_SIGNALS; i++)
  {
    String signalName = "Sine_";
    BlaeckTCP.addSignal(signalName + i, &sine);
  }

  // Register command handlers (new style)
  BlaeckTCP.onCommand("SwitchLED", onSwitchLED);
  BlaeckTCP.onCommand("SomeCommand", onSomeCommand);
  BlaeckTCP.onCommand("Print", onPrint);

  // Start listening for clients
  TelnetPrint = NetServer(SERVER_PORT);
  TelnetPrint.begin();
}

void loop()
{
  UpdateSineNumbers();

  /*- Keeps watching for commands from TCP clients and transmits the reply messages back to all
      connected clients (data messages only to permitted)
    - Sends data messages to permitted clients (0b11111101) at the user-set interval (<BlAECK.ACTIVATE,..>) */
  BlaeckTCP.tick();
}

void UpdateSineNumbers()
{
  sine = sin(millis() * 0.00005);
}

void onSwitchLED(const char *command, const char *const *params, byte paramCount)
{
  (void)command;
  // Setup the clients, you want to print to
  // From right to left: client #0, #1, .. , #7
  // 1: Print enabled
  // 0: Print disabled
  int clientMask = 0b11111011; // client #2: Print disabled

  if (paramCount < 1)
  {
    return;
  }
  int state = atoi(params[0]);
  if (state == 1)
  {
    digitalWrite(ledPin, HIGH);
    for (byte i = 0; i < MAX_CLIENTS; i++)
    {
      if (BlaeckTCP.Clients[i].connection.connected() && bitRead(clientMask, i) == 1)
      {
        BlaeckTCP.Clients[i].connection.println("LED is ON.");
      }
    }
    return;
  }
  if (state == 0)
  {
    digitalWrite(ledPin, LOW);
    for (byte i = 0; i < MAX_CLIENTS; i++)
    {
      if (BlaeckTCP.Clients[i].connection.connected() && bitRead(clientMask, i) == 1)
      {
        BlaeckTCP.Clients[i].connection.println("LED is OFF.");
      }
    }
    return;
  }
}

void onSomeCommand(const char *command, const char *const *params, byte paramCount)
{
  (void)command;
  (void)params;
  (void)paramCount;
  // Do something
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
