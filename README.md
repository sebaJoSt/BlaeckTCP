<a href="url"><img src="https://user-images.githubusercontent.com/388152/185908831-4eccf7a6-5f43-405d-b7fe-5225eeba302d.png" height="75"></a>
<a href="url"><img src="https://github.com/sebaJoSt/BlaeckTCP/assets/388152/15f6a932-2263-4453-9686-0ad9e36720fd"  alt="BlaeckTCP Logo SeeSaw Font" height="70"></a>
===



BlaeckTCP is a simple Arduino library to send binary (sensor) data via Ethernet/WiFi to your PC using the [Blaeck protocol](https://sebajost.github.io/blaeck-protocol/). The data can be sent periodically or requested on demand with [commands](#blaecktcp-commands).
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

See the [protocol documentation](https://sebajost.github.io/blaeck-protocol/protocol/commands) for the full list of commands and their parameters.

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
- `setBeforeWriteCallback(...)` before data is written
- `setClientConnectedCallback(...)` / `setClientDisconnectedCallback(...)` for client connection events
- `isClientDataEnabled(clientNo)` to query whether a client is allowed to receive data frames

When handling commands, use `CommandingClient` to reply to the sender of the current command.

Command parser defaults are architecture-aware:
- AVR (`__AVR__`): 48 command chars, 6 registered handlers (12 on larger-SRAM AVR such as the Mega 2560), 24 command-name chars, 10 params
- Non-AVR: 96 command chars, 12 registered handlers, 40 command-name chars, 10 params

See [Configuration](#configuration) to change them.

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

## Configuration

Compile-time settings (buffer sizes, command parser limits,
`BLAECK_ENABLE_COMMAND_META`, `BLAECK_TCP_NO_DELAY_DEFAULT`) are plain `#define`s
with `#ifndef` guards, so any value you define first wins.

> [!IMPORTANT]
> An override **must reach every translation unit** — your sketch *and*
> `BlaeckTCP.cpp`. These values size members of `class BlaeckTCP`, so a setting
> seen by only one of them gives the class two different layouts (an ODR
> violation) and corrupts memory silently. It is all-or-nothing, never half.
>
> In particular, **do not** `#define` them at the top of your `.ino` — that
> reaches your sketch only, never the library.

### PlatformIO

```ini
build_flags = -DBLAECK_BUFFER_SIZE=512
```

Reaches every unit. Nothing else to do.

### Arduino IDE / arduino-cli

A `BlaeckTCPConfig.h` in your sketch folder is **not** found by default: the
sketch folder is not on the compiler's include path, so the `__has_include` in
`BlaeckTCP.h` finds nothing and your settings are silently ignored. There is no
IDE preference and no `sketch.yaml` key for compiler flags — only the core's own
config files can add them.

This is an Arduino build-system limitation, not a library one: a sketch-local
settings header was requested in 2015 ([arduino-builder#15](https://github.com/arduino/arduino-builder/issues/15),
closed) and libraries still cannot extend the include path
([arduino-cli#501](https://github.com/arduino/arduino-cli/issues/501), open since 2019).

Three ways round it:

**a) Build with arduino-cli.** Put `BlaeckTCPConfig.h` next to your `.ino`:

```
MySketch/
  MySketch.ino
  BlaeckTCPConfig.h
```

```bash
arduino-cli compile --fqbn <board> \
  --build-property "compiler.cpp.extra_flags=-I{build.source.path}" \
  MySketch
```

`{build.source.path}` expands to the sketch folder, so the config is found by
every unit. Nothing in your Arduino installation is touched and the setting
stays with the sketch — the only option your CI can reproduce exactly.

**b) Put the config inside the library**, at
`libraries/BlaeckTCP/src/BlaeckTCPConfig.h`. That folder is already on the
include path, so every unit sees it, in the IDE as well as the CLI. The catch is
that it belongs to the library, not the sketch: it applies to every sketch you
build, and a library update overwrites it.

**c) Stay in the IDE, per sketch.** Same sketch-folder layout as (a), plus a
`platform.local.txt` next to your core's `platform.txt` containing:

```
compiler.cpp.extra_flags=-I{build.source.path}
```

On Windows, for the AVR core, that file goes at
`C:\Users\<you>\AppData\Local\Arduino15\packages\arduino\hardware\avr\1.8.8\platform.local.txt`
(macOS/Linux: `~/.arduino15/packages/…`, same tail). This is per core — repeat it
for esp32, samd, renesas_uno, … Without this file the config is silently ignored.

> [!NOTE]
> Use `compiler.cpp.extra_flags` rather than `build.extra_flags` here. 13 of the
> 27 boards in `arduino:avr` set their own `build.extra_flags={build.usb_flags}`
> (Leonardo, Micro, Yún and the other 32u4 boards), and a board-specific value
> overrides the platform-level one — so a `build.extra_flags` line would silently
> do nothing on those boards. No board overrides `compiler.cpp.extra_flags`.
>
> If you scope it per board with `boards.local.txt` instead (e.g.
> `mega.compiler.cpp.extra_flags=…`) and the board already defines the property
> you are setting, append to it rather than replacing it.

### Example config file

```CPP
// BlaeckTCPConfig.h
#define BLAECK_BUFFER_SIZE 512
#define BLAECK_COMMAND_MAX_CHARS_DEFAULT 128
#define BLAECK_COMMAND_MAX_HANDLERS_DEFAULT 8
#define BLAECK_COMMAND_MAX_NAME_CHARS_DEFAULT 48
#define BLAECK_COMMAND_MAX_PARAMS_DEFAULT 16
#define BLAECK_TCP_NO_DELAY_DEFAULT false  // disable Nagle optimization
```

## Protocol

Full protocol specification with version history: [sebajost.github.io/blaeck-protocol](https://sebajost.github.io/blaeck-protocol/blaecktcp/overview)

