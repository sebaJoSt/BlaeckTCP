/*
  CommandsEthernet.ino

  This is a sample sketch to show how to use BlaeckTCP to
  implement your own command. The example implements
  the command <SwitchLED> which turns the on-board LED on
  or off.

  Copyright (c) by Sebastian Strobl,

  The command syntax for implementing your own commands:

    Command:         <COMMAND,PARAMETER01,PARAMETER02,...,PARAMETER10>
    StringCommand:   <COMMAND, STRING01  ,PARAMETER02,...,PARAMETER10>
                     <-           --  max. 64 chars ---             ->
                     <-         --  max. 10 Parameters ---          ->

    COMMAND:         String
    PARAMETER01..10  Int 16 Bit
    STRING01:        max. 15 chars
    Start Marker*:    <
    End Marker*:      >
    Separation*:      ,

      * Not allowed in COMMAND, PARAMETER & STRING01

    Empty PARAMETER are not allowed,
    e.g. don't do: <COMMAND,,PARAMETER02>   <- PARAMETER02 will be stored in PARAMETER01
               do: <COMMAND,PARAMETER01,PARAMETER02>

  The circuit:
    - Ethernet shield attached to pins 10, 11, 12, 13
    - Use the on-board LED
      Note: Most Arduinos have an on-board LED you can control. On the UNO and MEGA
            it is attached to digital pin 13. LED_BUILTIN is set to the correct LED pin
            independent of which board is used.

  Using the sketch:
    - Upload the sketch to your Arduino.
    - Open a Telnet Client (e.g. PuTTY) and connect to IP Adress 192.168.1.177 (Port 23)
    - Type the following command and press enter:
        <SwitchLED,1>    Turn on the LED
        <SwitchLED,0>    Turn off the LED
*/

#include <SPI.h>
#include <Ethernet.h>
#include "BlaeckTCP.h"

#define MAX_CLIENTS 8
#define SERVER_PORT 23

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

// Sets the pin number:
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

  // Setup BlaeckTCP
  // Create a server that listens for incoming connections on the specified port.
  BlaeckTCP.begin(
      SERVER_PORT, // Port to listen on
      MAX_CLIENTS, // Maximal number of allowed clients
      &Serial,     // Serial reference, used for debugging
      0            // Maximal signal count used;
  );

  // Setup read callback function by passing a function
  BlaeckTCP.attachRead(startCommand);
}

void loop()
{
  /* Keeps watching for TCP input and fires the callback function
     startCommand when a input with the correct syntax is detected.
     Instead of BlaeckTCP.read you can use BlaeckTCP.tick
     if you want to add signals and write them in a user-set interval.
  */
  BlaeckTCP.read();
}

// Implement the function, don't forget the arguments
void startCommand(char *command, int *parameter, char *string01)
{
  /* Compares the user input to the string "SwitchLED"
     strcmp takes the two strings to be compared as parameters
     and returns 0 if the strings are equal*/
  if (strcmp(command, "SwitchLED") == 0)
  {
    // parameter[0] is the first parameter after the command: PARAMETER01
    if (parameter[0] == 1)
    {
      // Turns on the LED
      digitalWrite(ledPin, HIGH);

      // Print to active client (= client, which sent the command)
      BlaeckTCP.ActiveClient.println("LED is ON.");
    }

    if (parameter[0] == 0)
    {
      // Turns off the LED
      digitalWrite(ledPin, LOW);

      // Print to active client (= client, which sent the command)
      BlaeckTCP.ActiveClient.println("LED is OFF.");
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
