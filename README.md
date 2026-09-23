<a href="url"><img src="https://user-images.githubusercontent.com/388152/185908831-4eccf7a6-5f43-405d-b7fe-5225eeba302d.png" height="75"></a>
<a href="url"><img src="https://github.com/sebaJoSt/BlaeckTCP/assets/388152/15f6a932-2263-4453-9686-0ad9e36720fd"  alt="BlaeckTCP Logo SeeSaw Font" height="70"></a>
===

BlaeckTCP is an Arduino library. It sends any value your sketch holds - sensor readings,
calculated results, text - over Ethernet or WiFi as binary data, using the
[Blaeck protocol](https://sebajost.github.io/blaeck-protocol/).

It is the same library as [BlaeckSerial](https://github.com/sebaJoSt/BlaeckSerial), over a
network instead of a serial port. A sketch registers the same signals, commands, state
channels and events, and a host receives the same frames. Only `begin()` differs.

It is the first part of a chain:

1. **Your Arduino sketch** uses BlaeckTCP to register each variable it sends as a *signal* -
   a temperature, a counter, a switch position. You can also register the commands the board
   accepts and the events it fires.
2. **Loggbok**, a data logging tool, connects to the board over TCP, reads the signals and
   stores them in a database. It is also an MQTT bridge: it publishes the signals and commands
   to a broker.
3. **Home Assistant** subscribes to that broker and creates one entity for each: a sensor for
   a signal, a slider or button for a command.

Loggbok is an internal tool and is not publicly released. The protocol is documented, so you
can write your own host.

## A first sketch

This sketch sends two values from a Mega with an Ethernet shield:

```cpp
#include <SPI.h>
#include <Ethernet.h>
#include <BlaeckTCP.h>

BlaeckTCP Blaeck;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
const uint16_t SERVER_PORT = 23;

float temperature;
long  pressure;

void setup()
{
  Ethernet.begin(mac);
  Blaeck.begin(SERVER_PORT);

  Blaeck.DeviceName = "Weather Station";

  Blaeck.addSignal(F("Temperature"), &temperature);
  Blaeck.addSignal(F("Pressure"), &pressure);
}

void loop()
{
  ReadSensors();

  Blaeck.tick();
}
```

Three calls do the work:

- `begin(SERVER_PORT)` starts a TCP server on that port, once the network is up.
- `addSignal(...)` registers a variable. BlaeckTCP keeps a pointer to it and reads it whenever
  it sends data, so you only have to keep the variable up to date.
- `tick()` accepts connections, reads incoming commands and sends the values when they are
  due. Call it in every `loop()`.

The host decides how often data is sent. It sends `<BLAECK.ACTIVATE,1000>` to get one frame
per second, and `<BLAECK.DEACTIVATE>` to stop.

## Hosts and terminals

Several connections can be open at once, and each is one of two kinds:

- A **host**, such as Loggbok, speaks the protocol. A connection becomes a host by sending a
  command starting with `BLAECK.`, and receives frames from then on.
- A **terminal**, such as PuTTY, is for a person. It receives the text your sketch prints to
  `Blaeck.Terminal`, never a frame, and the commands you type in it run as usual.

```cpp
Blaeck.begin(SERVER_PORT).withDebugStream(&Blaeck.Terminal);   // library messages on the terminal
Blaeck.Terminal.println("LED is ON.");                         // your own text
```

[docs/network.md](docs/network.md) explains connections in full.

## Documentation

Everything about signals, commands, state channels and events is the same as in BlaeckSerial,
and documented there. Read `begin(&Serial)` in those pages as `begin(SERVER_PORT)`.

| Guide | What it covers |
|---|---|
| [Network](docs/network.md) | Hosts and terminals, connections, boards, OTA updates |
| [Configuration](docs/configuration.md) | Settings that only BlaeckTCP has |
| [Signals](https://github.com/sebaJoSt/BlaeckSerial/blob/master/docs/signals.md) | Registering values, naming them, and describing how they are shown |
| [Commands](https://github.com/sebaJoSt/BlaeckSerial/blob/master/docs/commands.md) | Reacting to commands, and declaring them as controls |
| [State channels](https://github.com/sebaJoSt/BlaeckSerial/blob/master/docs/state-channels.md) | Reporting a value that is displayed but not logged |
| [Events](https://github.com/sebaJoSt/BlaeckSerial/blob/master/docs/events.md) | Reporting that something happened |
| [Sending data](https://github.com/sebaJoSt/BlaeckSerial/blob/master/docs/sending-data.md) | Intervals, sending it yourself, timestamps, buffered writes |
| [Table sizes](https://github.com/sebaJoSt/BlaeckSerial/blob/master/docs/configuration.md) | Table sizes and the switches shared with BlaeckSerial |

## Examples

The examples are in `examples/`. In the Arduino IDE, open them with
**File > Examples > BlaeckTCP**.

- **`Basic`** is the smallest sketch, on the same boards as the topic examples.
- **`MoreBoards`** holds boards outside those four, each getting online on its own: `WiFi`
  on the UNO R4 WiFi and ESP32 boards, and `ESP32C6BugBoard`.
- **Topic examples** match BlaeckSerial's, one each, and run on the Mega and the Giga with an
  Ethernet shield, the ESP32-PoE and the WT32-ETH01: `SineGenerator`, `Commands`,
  `StateChannels`, `EventChannels`, `WriteModes`, `TimeStampModes` and `WaveformGenerator`.
  **WaveformGenerator** uses every feature, and also takes updates over the network.

## Reference

Every method is documented in `src/BlaeckTCP.h` and `src/BlaeckCore.h`, with an example. Your
editor shows it when you hover over a call.

The frame formats and the connection rules are described in the
[Blaeck protocol specification](https://sebajost.github.io/blaeck-protocol/blaecktcp/overview).

## Help and licence

For questions and bug reports, see [SUPPORT.md](SUPPORT.md). To contribute, see
[CONTRIBUTING.md](CONTRIBUTING.md). BlaeckTCP is released under the MIT licence
([LICENSE.md](LICENSE.md)).
