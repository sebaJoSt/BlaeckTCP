/*
  SineGeneratorAndCommandsEthernet.ino

  This is a sample sketch combining SineGeneratorEthernet.ino and CommandsEthernet.ino

  Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13

 created by Sebastian Strobl
 */

#include <SPI.h>
#include <Ethernet.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define MAX_CLIENTS 8
#define SERVER_PORT 23
#define MAX_SIGNALS 10

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

// Signals
float sine;

// Sets the LED pin number
const int ledPin = LED_BUILTIN;

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
    Serial.println("Ethernet shield was not found. Sorry, can't run without hardware. :(");
    while (true)
    {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF)
  {
    Serial.println("Ethernet cable is not connected.");
  }

  Serial.print("BlaeckTCP Server: ");
  Serial.print(Ethernet.localIP());
  Serial.print(":");
  Serial.println(SERVER_PORT);

  // Setup the clients, which are allowed to receive Blaeck Data
  // 1: Client receives Blaeck Data
  // 0: Client does not received Blaeck Data
  byte blaeckWriteClientMask[MAX_CLIENTS] = {1, 0, 1, 1, 1, 1, 1, 1};

  // Setup BlaeckTCP
  // Create a server that listens for incoming connections on the specified port.
  BlaeckTCP.begin(
      SERVER_PORT,          // Port to listen on
      MAX_CLIENTS,          // Maximal number of allowed clients
      &Serial,              // Serial reference, used for debugging
      MAX_SIGNALS,          // Maximal signal count used;
      blaeckWriteClientMask // Clients allowed to receive Blaeck Data
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

  // Setup read callback function by passing a function
  BlaeckTCP.attachRead(startCommand);
}

void loop()
{
  UpdateSineNumbers();

  /*Keeps watching for commands from TCP client and
    transmits the data back to client at the user-set interval*/
  BlaeckTCP.tick();
}

void UpdateSineNumbers()
{
  sine = sin(millis() * 0.00005);
}

// Implement the function, don't forget the arguments
void startCommand(char *command, int *parameter, char *string01)
{
  // Setup the clients, you want to print to.
  // 1: Print enabled
  // 0: Print disabled
  static byte clientMask[MAX_CLIENTS] = {1,  // client #0
                                         1,  // client #1
                                         0,  // client #2: Print disabled
                                         1,  // client #3
                                         1,  // client #4
                                         1,  // client #5
                                         1,  // client #6
                                         1}; // client #7

  if (strcmp(command, "SwitchLED") == 0)
  {
    // parameter[0] is the first parameter after the command: PARAMETER01
    if (parameter[0] == 1)
    {
      // Turns on the LED
      digitalWrite(ledPin, HIGH);

      for (byte i = 0; i < MAX_CLIENTS; i++)
      {
        if (BlaeckTCP.Clients[i].connected() && clientMask[i] == 1)
        {
          // Print to clients connected and enabled in clientMask
          BlaeckTCP.Clients[i].println("LED is ON.");
        }
      }
    }

    if (parameter[0] == 0)
    {
      // Turns off the LED
      digitalWrite(ledPin, LOW);

      for (byte i = 0; i < MAX_CLIENTS; i++)
      {
        if (BlaeckTCP.Clients[i].connected() && clientMask[i] == 1)
        {
          // Print to clients connected and enabled in clientMask
          BlaeckTCP.Clients[i].println("LED is OFF.");
        }
      }
    }
  }

  /* Here you can add more commands:*/
  if (strcmp(command, "SomeCommand") == 0)
  {
    // Do something
  }

  /* Exemplary command using the string parameter STRING01:
     Example: <Print,Bye Bye,1>
  */
  if (strcmp(command, "Print") == 0)
  {
    if (parameter[1] == 0)
    {
      // Print to active client (= client, which sent the command)
      BlaeckTCP.ActiveClient.println(string01);
    }
    if (parameter[1] == 1)
    {
      // Print to active client (= client, which sent the command)
      BlaeckTCP.ActiveClient.print(string01);
      BlaeckTCP.ActiveClient.println(" Miss American Pie");
      BlaeckTCP.ActiveClient.println("Drove my Chevy to the levee but the levee was dry");
      BlaeckTCP.ActiveClient.println("And them good ole boys were drinking whiskey and rye");
      BlaeckTCP.ActiveClient.println("Singin' this'll be the day that I die");
      BlaeckTCP.ActiveClient.println("This'll be the day that I die");
    }
  }
}
