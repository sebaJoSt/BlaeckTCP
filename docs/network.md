# Network

What BlaeckTCP adds to BlaeckSerial: a server with several connections, and the two kinds
of connection it tells apart.

## Starting the server

Bring the network up first, then start BlaeckTCP on a port:

```cpp
void setup()
{
  Ethernet.begin(mac);

  Blaeck.begin(SERVER_PORT)
      .withClients(4)
      .withSignals(50)
      .withDebugStream(&Blaeck.Terminal);
}
```

`withClients()` sets how many connections are accepted at once, hosts and terminals
together; the default is 4. Each connection takes a receive buffer, 128 bytes on a Mega. A
connection beyond the limit is closed straight away. Like the table sizes, the number is
fixed once the first `read()` or `tick()` has run.

## Hosts and terminals

Every connection starts as a **terminal**. It becomes a **host** when it sends a command
whose name starts with `BLAECK.`, such as `<BLAECK.GET_DEVICES>`, and stays one until it
disconnects.

| | Host | Terminal |
|---|---|---|
| Typical client | Loggbok, blaecktcpy | PuTTY, telnet |
| Receives frames | yes | never |
| Receives text from `Blaeck.Terminal` | no | yes |
| Its commands | run and acknowledged | run, not acknowledged |

Hosts share one device, as they would a serial port: `ACTIVATE` sets one interval for all of
them, `PAUSE_WRITES` holds back frames to all of them, and data, state values and events go to
every host. The answer to a request - a catalog, the device frame, an acknowledgement - goes
only to the host that asked.

The [Connections page](https://sebajost.github.io/blaeck-protocol/protocol/connections) of
the protocol specification is the full contract, for anyone writing a host.

## Blaeck.Terminal

`Blaeck.Terminal` prints to every connected terminal and to nothing else. Use it like
`Serial`:

```cpp
void onLED(const char *command, const char *const *params, byte paramCount)
{
  setLed(atoi(params[0]) == 1);
  Blaeck.Terminal.println(ledState ? "LED is ON." : "LED is OFF.");
}
```

Passed to `withDebugStream()`, it also shows what the library refuses and why, every command
that arrives, and connections opening and closing. That makes a terminal the easiest way to
see what a device is doing while a host is connected.

A terminal that connects but never reads can fill its send buffer, and a write to it may then
wait. Short lines don't get there.

## When a connection drops

A connection that closes properly frees its slot at once, and
`setClientDisconnectedCallback()` is called. One that dies without closing - a pulled cable,
a laptop going to sleep - is noticed only when the network stack gives up on it.

A host that reconnects starts as a terminal again, and receives nothing until it sends a
`BLAECK.` command. Loggbok does that as soon as it reconnects, so data continues, and the
device's interval and pause are unchanged. If the board restarted while the connection was
down, the restart notice goes to the first host that connects afterwards.

`setClientConnectedCallback()` and `setClientDisconnectedCallback()` receive the connection's
slot, starting at 0. They are for the sketch's own use, such as a status LED.

## Boards

`Basic` and the topic examples run on these boards through a shared `NetworkSetup.h` tab:

| Board | Network |
|---|---|
| Arduino Mega 2560 | Ethernet shield (W5100/W5500) |
| Arduino Giga R1 | Ethernet shield |
| Olimex ESP32-PoE | built-in Ethernet |
| WT32-ETH01 | built-in Ethernet |

The `more` folder includes additional networking examples:
[WiFi](../examples/more/WiFi) gets the UNO R4 WiFi and ESP32 boards online, and
[ESP32C6BugBoard](../examples/more/ESP32C6BugBoard) sets the pins for the
ESP32-C6-Bug's Ethernet add-on. These use their own network setup.

The hardware name defaults to the selected build target, with a friendly name for recognised
boards or the core's board identifier otherwise. It is not physical board detection.
`DeviceHWVersion` can still describe custom hardware: the C6 Bug example overrides it because
its generic ESP32C6 build target cannot identify that board or its wiring.

## Network timestamps

The default `BLAECK_NO_TIMESTAMP` mode lets the host timestamp data on arrival.
`Blaeck.setTimestampMode(BLAECK_MICROS)` uses device uptime instead; neither needs a clock
source. [TimestampsNTP](../examples/more/TimestampsNTP) uses the ESP32 network clock on an
ESP32-PoE or WT32-ETH01. It waits for synchronization before starting the TCP server and reports
failed attempts on the serial monitor, rather than sending readings dated 1970. It needs
network access to `pool.ntp.org`.

## A serial device on the network

[BridgeESP32PoE](../examples/more/BridgeESP32PoE) forwards bytes unchanged between one TCP
connection and a UART. The device behind it runs BlaeckSerial; the bridge does not use
BlaeckTCP or interpret its frames.

## Updates over the network

`WaveformGenerator` turns on updates over the network and Bonjour, with
`#define NETWORK_WITH_SERVICES` before `#include "NetworkSetup.h"`. Any other topic example can do
the same. Each board needs a one-time setup first; see the
[WaveformGenerator README](../examples/WaveformGenerator/README.md).

Giga OTA storage is included in `NetworkSetup.h`; no additional sketch file is needed.
Its `Arduino_Portenta_OTA` dependency is required only for Giga builds with services enabled.
