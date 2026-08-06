/*
  WaveformGeneratorEthernet.ino

  A dashboard-friendly demo for the BlaeckTCP -> Loggbok -> MQTT bridge, running on an
  Arduino with an Ethernet Shield (W5100/W5500, Server) and streamed to your PC (Client)
  over Ethernet.

  It generates one fully controllable waveform. Frequency, amplitude, offset and waveform
  shape are all set over MQTT commands. The commands are registered with typed helpers
  (onNumberCommand / onSelectCommand / onSwitchCommand / onButtonCommand) so the device is
  self-describing: it advertises range, unit, options and the mirrored signal in a 0xE0
  "Command List" frame, which Loggbok turns into Home Assistant MQTT Discovery entities.
  Out-of-range values are rejected by the library (and reported on the debug stream); each
  accepted value is written back to its signal, so a dashboard always shows the value the
  device actually applied. A read-only string signal (WaveName) mirrors the selected shape
  as human-readable text, and a writable free-text command (SET_LABEL / DeviceLabel) shows a
  Home Assistant text entity round-tripping an arbitrary string via onTextCommand.

  Author: Sebastian Strobl, https://github.com/sebaJoSt/BlaeckTCP

  Circuit:
    Ethernet shield attached to pins 10, 11, 12, 13

  --- DASHBOARD MAPPING (Loggbok topic prefix: "loggbok" table name: "wave") ---
    Topic                       Widget          Meaning
    loggbok/wave/Output         chart           live generated sample
    loggbok/wave/Frequency      slider / text   wave frequency [Hz]
    loggbok/wave/Amplitude      slider / text   peak amplitude
    loggbok/wave/Offset         slider / text   DC offset
    loggbok/wave/Waveform       text/select     0=Sine 1=Square 2=Triangle 3=Sawtooth
    loggbok/wave/WaveName       text sensor     current waveform shape name (mirrors Waveform)
    loggbok/wave/DeviceLabel    text            free-text label (set via SET_LABEL)
    loggbok/wave/Enabled        switch          output on/off (off -> Output = Offset)

  --- COMMANDS (publish to loggbok/<table>/_cmd/<NAME>, or loggbok/_all/_cmd/<NAME>) ---
    SET_FREQ    <0..2>      frequency [Hz]            -> Frequency  (HA number, step 0.01)
    SET_AMP     <0..100>    peak amplitude            -> Amplitude  (HA number, step 0.1)
    SET_OFFSET  <-100..100> DC offset                 -> Offset     (HA number, step 0.1)
    SET_WAVE    <0..3>      Sine/Square/Triangle/Saw  -> Waveform   (HA select; name or index)
    SET_ENABLE  <0|1>       output on/off             -> Enabled    (HA switch)
    SET_LABEL   <text>      free-text device label    -> DeviceLabel (HA text, max 32)
    STATUS                  print info to serial                    (HA button)

  Setup:
    Upload the sketch to your board. Adjust the MAC/IP for your local network below.

  Loggbok CLI (log fast enough to resolve the wave, e.g. 20 ms):
    Replace <device-ip> with the IP printed on the serial monitor (default 192.168.1.177).

    lgbk log --tcp <device-ip>:23 --table wave --signals * --interval 20 \
      --mqtt --mqtt-endpoint mqtt://127.0.0.1:1884

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

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

//---SIGNALS (fixed set -> safe to control while logging)
float Output = 0.0;
float Frequency = 1.0; // [Hz]
float Amplitude = 1.0;
float Offset = 0.0;
byte Waveform = 0; // 0=Sine, 1=Square, 2=Triangle, 3=Sawtooth
bool Enabled = true;
// Human-readable shape names for the WaveName string signal (-> HA text sensor).
// Single source for the sensor text; keep in sync with the SET_WAVE options CSV in setup().
const char *const WAVE_NAMES[] = {"Sine", "Square", "Triangle", "Sawtooth"};
// Free-text label the user can set from Home Assistant (-> HA text entity via onTextCommand).
// Owned buffer; max 32 chars (+ null). The value arrives percent-decoded from the host.
char DeviceLabel[33] = "wave-gen";

//---COMMAND HANDLERS
void onSetFreq(const char *command, const char *const *params, byte paramCount);
void onSetAmp(const char *command, const char *const *params, byte paramCount);
void onSetOffset(const char *command, const char *const *params, byte paramCount);
void onSetWave(const char *command, const char *const *params, byte paramCount);
void onSetEnable(const char *command, const char *const *params, byte paramCount);
void onSetLabel(const char *command, const char *const *params, byte paramCount);
void onStatus(const char *command, const char *const *params, byte paramCount);

//---GENERATOR STATE
double phase = 0.0; // normalized phase 0..1
unsigned long lastMicros = 0;

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
      SERVER_PORT  // TCP server port
  );

  BlaeckTCP.DeviceName = "Waveform Generator Demo TCP";
  BlaeckTCP.DeviceHWVersion = "Arduino Mega 2560 Rev3";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  BlaeckTCP.addSignal("Output", &Output);
  BlaeckTCP.addSignal("Frequency", &Frequency);
  BlaeckTCP.addSignal("Amplitude", &Amplitude);
  BlaeckTCP.addSignal("Offset", &Offset);
  BlaeckTCP.addSignal("Waveform", &Waveform);
  BlaeckTCP.addSignal("Enabled", &Enabled);
  BlaeckTCP.addSignal("WaveName", (char *)WAVE_NAMES[Waveform]);
  BlaeckTCP.addSignal("DeviceLabel", DeviceLabel);

  BlaeckTCP.onNumberCommand("SET_FREQ", onSetFreq, F("Frequency"), 0.0f, 2.0f, 0.01f, F("Hz"));
  BlaeckTCP.onNumberCommand("SET_AMP", onSetAmp, F("Amplitude"), 0.0f, 100.0f, 0.1f);
  BlaeckTCP.onNumberCommand("SET_OFFSET", onSetOffset, F("Offset"), -100.0f, 100.0f, 0.1f);
  BlaeckTCP.onSelectCommand("SET_WAVE", onSetWave, F("Waveform"), F("Sine,Square,Triangle,Sawtooth"));
  BlaeckTCP.onSwitchCommand("SET_ENABLE", onSetEnable, F("Enabled"));
  BlaeckTCP.onTextCommand("SET_LABEL", onSetLabel, F("DeviceLabel"), 32);
  BlaeckTCP.onButtonCommand("STATUS", onStatus);

  lastMicros = micros();
}

void loop()
{
  UpdateWaveform();

  /*- Keeps watching for commands from TCP clients and transmits the reply messages back to all
      connected clients
    - Sends data messages to all clients at the user-set interval (<BlAECK.ACTIVATE,..>) */
  BlaeckTCP.tick();
}

void UpdateWaveform()
{
  unsigned long now = micros();
  double dt = (now - lastMicros) * 1e-6; // [s]
  lastMicros = now;

  if (!Enabled)
  {
    Output = Offset;
    return;
  }

  // Advance and wrap the normalized phase (0..1).
  phase += (double)Frequency * dt;
  phase -= floor(phase);

  double w = 0.0;
  switch (Waveform)
  {
  case 1: // Square
    w = (phase < 0.5) ? 1.0 : -1.0;
    break;
  case 2: // Triangle: +1 at phase 0, -1 at phase 0.5
    w = 1.0 - 4.0 * fabs(phase - 0.5);
    break;
  case 3: // Sawtooth: -1 .. +1 ramp
    w = 2.0 * phase - 1.0;
    break;
  default: // Sine
    w = sin(2.0 * PI * phase);
    break;
  }

  Output = Offset + Amplitude * w;
}

// Rounds to a fixed number of decimal places, cleaning up tiny float rounding noise from atof()
// (e.g. "0.15" -> 0.14999999...).
float roundToDecimals(float value, byte decimals)
{
  float scale = pow(10, decimals);
  return roundf(value * scale) / scale;
}

void onSetFreq(const char *command, const char *const *params, byte paramCount)
{
  if (paramCount >= 1 && params[0][0] != '\0')
  {
    Frequency = roundToDecimals((float)atof(params[0]), 4);
    BlaeckTCP.write("Frequency", Frequency);
  }
}

void onSetAmp(const char *command, const char *const *params, byte paramCount)
{
  if (paramCount >= 1 && params[0][0] != '\0')
  {
    Amplitude = roundToDecimals((float)atof(params[0]), 4);
    BlaeckTCP.write("Amplitude", Amplitude);
  }
}

void onSetOffset(const char *command, const char *const *params, byte paramCount)
{
  if (paramCount >= 1 && params[0][0] != '\0')
  {
    Offset = roundToDecimals((float)atof(params[0]), 4);
    BlaeckTCP.write("Offset", Offset);
  }
}

void onSetWave(const char *command, const char *const *params, byte paramCount)
{
  if (paramCount >= 1 && params[0][0] != '\0')
  {
    Waveform = (byte)atoi(params[0]);
    BlaeckTCP.write("Waveform", Waveform);
    BlaeckTCP.write("WaveName", (char *)WAVE_NAMES[Waveform]);
  }
}

void onSetEnable(const char *command, const char *const *params, byte paramCount)
{
  Enabled = paramCount >= 1 && atoi(params[0]) == 1;
  BlaeckTCP.write("Enabled", Enabled);
}

void onSetLabel(const char *command, const char *const *params, byte paramCount)
{
  if (paramCount >= 1)
  {
    // params[0] is already percent-decoded by BlaeckTCP; copy into our owned buffer.
    strncpy(DeviceLabel, params[0], sizeof(DeviceLabel) - 1);
    DeviceLabel[sizeof(DeviceLabel) - 1] = '\0';
    BlaeckTCP.write("DeviceLabel", DeviceLabel);
  }
}

void onStatus(const char *command, const char *const *params, byte paramCount)
{
  Serial.print(F("Enabled=")), Serial.print(Enabled);
  Serial.print(F(" Wave=")), Serial.print(Waveform);
  Serial.print(F(" Freq=")), Serial.print(Frequency);
  Serial.print(F(" Amp=")), Serial.print(Amplitude);
  Serial.print(F(" Offset=")), Serial.println(Offset);
}
