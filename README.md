<a href="url"><img src="https://user-images.githubusercontent.com/388152/185908831-4eccf7a6-5f43-405d-b7fe-5225eeba302d.png" height="75"></a>
<a href="url"><img src="https://github.com/sebaJoSt/BlaeckTCP/assets/388152/15f6a932-2263-4453-9686-0ad9e36720fd"  alt="BlaeckTCP Logo SeeSaw Font" height="70"></a>
===



BlaeckTCP is a simple Arduino library to send binary (sensor) data via Ethernet/WiFi to your PC. The data can be sent periodically or requested on demand with [commands](#blaecktcp-commands).
Also included is a message parser which reads input in the syntax of `<HelloWorld, 12, 47>`. You can register exact command handlers (`onCommand`) and a catch-all handler (`onAnyCommand`) in your sketch.

## Getting Started

Clone this repository into `Arduino/Libraries` or use the built-in Arduino IDE Library manager to install
a copy of this library. You can find more detail about installing libraries 
[here, on Arduino's website](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-installing-a-library).

(Open BasicEthernet Example under `File -> Examples -> BlaeckTCP` for reference)

```CPP
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <BlaeckTCP.h>
```
### Instantiate BlaeckTCP
```CPP
BlaeckTCP BlaeckTCP;
```
### Initialize Serial & BlaeckTCP
```CPP
void setup()
{
  Serial.begin(9600);

   BlaeckTCP.begin(
      MAX_CLIENTS,  // Maximal number of allowed clients
      &Serial,      // Serial reference, used for debugging
      2,            // Maximal signal count used;
      0b11111101,   // Clients allowed to receive Blaeck Data; from right to left: client #0, #1, .. , #7
      SERVER_PORT   // TCP server port
  );
}
```

### Add signals
```CPP
BlaeckTCP.addSignal("Small Number", &randomSmallNumber);
BlaeckTCP.addSignal("Big Number", &randomBigNumber);
```

If more signals are added than configured in `begin(...)`, additional `addSignal(...)`
calls are ignored. You can inspect this with:
```CPP
if (BlaeckTCP.hasSignalOverflow()) {
  Serial.print("Dropped addSignal calls: ");
  Serial.println(BlaeckTCP.getSignalOverflowCount());
}
```

### Update your variables and don't forget to `tick()`!
```CPP
void loop()
{
  UpdateYourVariables();

 /*Keeps watching for commands from TCP client and
     transmits the data back to client at the user-set interval*/
  BlaeckTCP.tick();
}
```

## BlaeckTCP commands

Here's a full list of the commands handled by this library:

| Command                      | Description                                                                      |
| ---------------------------- | -------------------------------------------------------------------------------- |
| `<BLAECK.GET_DEVICES>`       | Writes the device information. Optionally accepts client identity params (see [Client identity](#client-identity)). |
| `<BLAECK.WRITE_SYMBOLS> `    | Writes symbol list including datatype information.                               |
| `<BLAECK.WRITE_DATA> `       | Writes the binary data.                                                          |
| `<BLAECK.ACTIVATE,first,second,third,fourth byte>`| Activates writing the binary data in user-set interval [ms]<br />Min: 0ms  Max: 4294967295ms<br /> e.g. `<BLAECK.ACTIVATE,96,234>` The data is written every 60 seconds (60 000ms)<br />first Byte: 0b01100000 = 96 DEC<br />second Byte: 0b11101010 = 234 DEC|
| `<BLAECK.DEACTIVATE> `       | Deactivates writing in intervals.                                                |

### Interval lock mode

By default, timed data is client-controlled (`BLAECK.ACTIVATE` / `BLAECK.DEACTIVATE`).
You can lock interval behavior from sketch code:

```CPP
// Fixed interval lock: always send every 500 ms, ignore ACTIVATE/DEACTIVATE
BlaeckTCP.setIntervalMs(500);

// Off lock: disable timed data and ignore ACTIVATE
BlaeckTCP.setIntervalMs(BLAECK_INTERVAL_OFF);

// Back to client control (default behavior)
BlaeckTCP.setIntervalMs(BLAECK_INTERVAL_CLIENT);
```

`setTimedData(...)` has been removed. Use `setIntervalMs(...)` instead.

### Command handler API

Available callbacks:
- `onCommand(...)` and `onAnyCommand(...)` for parsed incoming commands
- `setCommandCallback(...)` (deprecated, still supported with runtime warning)
- `setBeforeWriteCallback(...)` before data is written
- `setClientConnectedCallback(...)` / `setClientDisconnectedCallback(...)` for client connection events
- `isClientDataEnabled(clientNo)` to query whether a client is allowed to receive data frames

When handling commands, use `CommandingClient` to reply to the sender of the current command.

Command parser defaults are architecture-aware:
- AVR (`__AVR__`): 48 command chars, 4 registered handlers, 24 command-name chars, 10 params
- Non-AVR: 96 command chars, 12 registered handlers, 40 command-name chars, 10 params

These defaults can be overridden by placing a `BlaeckTCPConfig.h` file in your sketch folder:
```CPP
// BlaeckTCPConfig.h
#define BLAECK_BUFFER_SIZE 512
#define BLAECK_COMMAND_MAX_CHARS_DEFAULT 128
#define BLAECK_COMMAND_MAX_HANDLERS_DEFAULT 8
#define BLAECK_COMMAND_MAX_NAME_CHARS_DEFAULT 48
#define BLAECK_COMMAND_MAX_PARAMS_DEFAULT 16
#define BLAECK_TCP_NO_DELAY_DEFAULT false  // disable Nagle optimization
```

PlatformIO users can also use compiler flags in `platformio.ini`:
```ini
build_flags = -DBLAECK_BUFFER_SIZE=512
```

```CPP
void onSwitchLED(const char *command, const char *const *params, byte paramCount)
{
  if (paramCount < 1) return;
  int state = atoi(params[0]);
  digitalWrite(LED_BUILTIN, state == 1 ? HIGH : LOW);
}

void onAny(const char *command, const char *const *params, byte paramCount)
{
  // Optional catch-all hook
}

void setup()
{
  // ...
  BlaeckTCP.onCommand("SwitchLED", onSwitchLED);
  BlaeckTCP.onAnyCommand(onAny);
}
```

## Protocol

For the binary wire format, message keys, elements, status codes, client identity, data types, timestamp modes, and decoding examples, see [docs/protocol.md](docs/protocol.md).

Full protocol specification with version history: [sebajost.github.io/blaeck-protocol](https://sebajost.github.io/blaeck-protocol/blaecktcp/overview)

