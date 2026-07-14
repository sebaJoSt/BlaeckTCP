/*
  WaveformGeneratorESP32PoE.ino

  A dashboard-friendly demo for the BlaeckTCP -> Loggbok -> MQTT bridge, running on the
  Olimex ESP32-PoE-ISO Rev.L board (Server) and streamed to your PC (Client) over Ethernet.

  It generates one fully controllable waveform. Frequency, amplitude, offset and waveform
  shape are all set over MQTT commands. Each command writes the accepted value back to its
  signal, so a dashboard always shows the value the device actually applied (after
  clamping/rounding).

  Author: Sebastian Strobl, https://github.com/sebaJoSt/BlaeckTCP

  --- DASHBOARD MAPPING (Loggbok topic prefix: "loggbok" table name: "wave") ---
    Topic                       Widget          Meaning
    loggbok/wave/Output         chart           live generated sample
    loggbok/wave/Frequency      slider / text   wave frequency [Hz]
    loggbok/wave/Amplitude      slider / text   peak amplitude
    loggbok/wave/Offset         slider / text   DC offset
    loggbok/wave/Waveform       text/select     0=Sine 1=Square 2=Triangle 3=Sawtooth
    loggbok/wave/Enabled        switch          output on/off (off -> Output = Offset)

  --- COMMANDS (publish to loggbok/<table>/_cmd/<NAME>, or loggbok/_all/_cmd/<NAME>) ---
    SET_FREQ    <float>   frequency [Hz]            -> Frequency
    SET_AMP     <float>   peak amplitude            -> Amplitude
    SET_OFFSET  <float>   DC offset                 -> Offset
    SET_WAVE    <0..3>    Sine/Square/Triangle/Saw  -> Waveform
    SET_ENABLE  <0|1>     output on/off             -> Enabled
    STATUS                print info to serial

  Setup:
    Upload the sketch to your board. By default, DHCP is used.
    For a static IP, uncomment ETH.config(ip, gateway, subnet, dns) in setup().

  Loggbok CLI (log fast enough to resolve the wave, e.g. 20 ms):
    Replace <device-ip> with the IP printed on the serial monitor after ETH gets an IP.

    lgbk log --tcp <device-ip>:23 --table wave --signals * --interval 20 \
      --mqtt --mqtt-endpoint mqtt://127.0.0.1:1884

  More information on: https://github.com/sebaJoSt/BlaeckTCP
*/

// Important to be defined BEFORE including ETH.h for ETH.begin() to work.
// Example RMII LAN8720 (Olimex, etc.)
#ifndef ETH_PHY_MDC
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#if CONFIG_IDF_TARGET_ESP32
#define ETH_PHY_ADDR 0
#define ETH_PHY_MDC 23
#define ETH_PHY_MDIO 18
// #define ETH_PHY_POWER -1
// #define ETH_CLK_MODE  ETH_CLOCK_GPIO0_IN
#define ETH_PHY_POWER 12
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#elif CONFIG_IDF_TARGET_ESP32P4
#define ETH_PHY_ADDR 0
#define ETH_PHY_MDC 31
#define ETH_PHY_MDIO 52
#define ETH_PHY_POWER 51
#define ETH_CLK_MODE EMAC_CLK_EXT_IN
#endif
#endif

#include <ETH.h>
#include "BlaeckTCP.h"

#define EXAMPLE_VERSION "1.0"
#define SERVER_PORT 23
#define MAX_CLIENTS 8
#define MAX_SIGNALS 10

// Instantiate a new BlaeckTCP object
BlaeckTCP BlaeckTCP;

// Enter a static IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
IPAddress ip(192, 168, 1, 177);
IPAddress dns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

//---SIGNALS (fixed set -> safe to control while logging)
float Output = 0.0;
float Frequency = 1.0; // [Hz]
float Amplitude = 1.0;
float Offset = 0.0;
byte Waveform = 0; // 0=Sine, 1=Square, 2=Triangle, 3=Sawtooth
bool Enabled = true;

//---COMMAND HANDLERS
void onSetFreq(const char *command, const char *const *params, byte paramCount);
void onSetAmp(const char *command, const char *const *params, byte paramCount);
void onSetOffset(const char *command, const char *const *params, byte paramCount);
void onSetWave(const char *command, const char *const *params, byte paramCount);
void onSetEnable(const char *command, const char *const *params, byte paramCount);
void onStatus(const char *command, const char *const *params, byte paramCount);

//---GENERATOR STATE
double phase = 0.0; // normalized phase 0..1
unsigned long lastMicros = 0;

void onEvent(arduino_event_id_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    Serial.println("ETH Started");
    ETH.setHostname("WaveformGeneratorESP32_01");
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    Serial.println("ETH Connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    Serial.print("ETH MAC: ");
    Serial.print(ETH.macAddress());
    Serial.print(", IPv4: ");
    Serial.print(ETH.localIP());
    Serial.print(", ");
    Serial.print(ETH.subnetMask());
    Serial.print(", ");
    Serial.println(ETH.gatewayIP());
    Serial.print("BlaeckTCP Server: ");
    Serial.print(ETH.localIP());
    Serial.print(":");
    Serial.println(SERVER_PORT);
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    Serial.println("ETH Disconnected");
    break;
  case ARDUINO_EVENT_ETH_STOP:
    Serial.println("ETH Stopped");
    break;
  default:
    break;
  }
}

void setup()
{
  Serial.begin(9600);
  delay(500);

  // Register ETH event handler
  Network.onEvent(onEvent);

  // Initialize ETH
  ETH.begin();

  // Configure static IP
  // ETH.config(ip, gateway, subnet, dns);

  // Setup BlaeckTCP
  BlaeckTCP.begin(
      MAX_CLIENTS, // Maximal number of allowed clients
      &Serial,     // Serial reference, used for debugging
      MAX_SIGNALS, // Maximal signal count used;
      SERVER_PORT  // TCP server port
  );

  BlaeckTCP.DeviceName = "Waveform Generator Demo TCP";
  BlaeckTCP.DeviceHWVersion = "ESP32-PoE-ISO Rev.L";
  BlaeckTCP.DeviceFWVersion = EXAMPLE_VERSION;

  BlaeckTCP.addSignal("Output", &Output);
  BlaeckTCP.addSignal("Frequency", &Frequency);
  BlaeckTCP.addSignal("Amplitude", &Amplitude);
  BlaeckTCP.addSignal("Offset", &Offset);
  BlaeckTCP.addSignal("Waveform", &Waveform);
  BlaeckTCP.addSignal("Enabled", &Enabled);

  BlaeckTCP.onCommand("SET_FREQ", onSetFreq);
  BlaeckTCP.onCommand("SET_AMP", onSetAmp);
  BlaeckTCP.onCommand("SET_OFFSET", onSetOffset);
  BlaeckTCP.onCommand("SET_WAVE", onSetWave);
  BlaeckTCP.onCommand("SET_ENABLE", onSetEnable);
  BlaeckTCP.onCommand("STATUS", onStatus);

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
    Frequency = roundToDecimals(constrain((float)atof(params[0]), 0.0f, 50.0f), 4);
    BlaeckTCP.write("Frequency", Frequency);
  }
}

void onSetAmp(const char *command, const char *const *params, byte paramCount)
{
  if (paramCount >= 1 && params[0][0] != '\0')
  {
    Amplitude = roundToDecimals(constrain((float)atof(params[0]), 0.0f, 100.0f), 4);
    BlaeckTCP.write("Amplitude", Amplitude);
  }
}

void onSetOffset(const char *command, const char *const *params, byte paramCount)
{
  if (paramCount >= 1 && params[0][0] != '\0')
  {
    Offset = roundToDecimals(constrain((float)atof(params[0]), -100.0f, 100.0f), 4);
    BlaeckTCP.write("Offset", Offset);
  }
}

void onSetWave(const char *command, const char *const *params, byte paramCount)
{
  if (paramCount >= 1 && params[0][0] != '\0')
  {
    Waveform = (byte)constrain(atoi(params[0]), 0, 3);
    BlaeckTCP.write("Waveform", Waveform);
  }
}

void onSetEnable(const char *command, const char *const *params, byte paramCount)
{
  Enabled = paramCount >= 1 && atoi(params[0]) == 1;
  BlaeckTCP.write("Enabled", Enabled);
}

void onStatus(const char *command, const char *const *params, byte paramCount)
{
  Serial.print(F("Enabled=")), Serial.print(Enabled);
  Serial.print(F(" Wave=")), Serial.print(Waveform);
  Serial.print(F(" Freq=")), Serial.print(Frequency);
  Serial.print(F(" Amp=")), Serial.print(Amplitude);
  Serial.print(F(" Offset=")), Serial.println(Offset);
}
