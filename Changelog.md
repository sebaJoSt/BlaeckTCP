# Changelog

All notable changes to this project will be documented in this file.

## [6.0.0] - 2026-04-01

### Changed
- **Breaking change:** Data message format updated from `D1` (0xD1) to `D2` (0xD2)
- **Breaking change:** Timestamps are now 8 bytes (uint64) instead of 4 bytes (uint32)
  - `BLAECK_MICROS` mode: tracks `micros()` overflow internally, produces monotonic uint64 (no more ~71 minute wrap)
  - `BLAECK_UNIX` mode: callback returns microseconds since epoch directly (uint64)
- **Breaking change:** Timestamp parameter type changed from `unsigned long` to `unsigned long long` in all public write methods
- **Breaking change:** Timestamp callback signature changed from `unsigned long (*)()` to `unsigned long long (*)()`. For `BLAECK_UNIX` mode, the callback must now return microseconds since epoch instead of seconds.
- Renamed `BLAECK_RTC` to `BLAECK_UNIX` (`BLAECK_RTC` kept as deprecated alias)
- **Breaking change:** Device message key updated from `B5` to `B6` and device payload extended with `<DeviceType><Parent>` to align with current hub/server decoder format.
- D2 frame tail updated to `<StatusByte><StatusPayload><CRC32>` (CRC32 is now always present as a separate field).
- Added interval lock API aligned with blaecktcpy: `setIntervalMs(...)` with `BLAECK_INTERVAL_CLIENT`, `BLAECK_INTERVAL_OFF`, or fixed millisecond values.
  - When locked to fixed/off mode, incoming `BLAECK.ACTIVATE`/`BLAECK.DEACTIVATE` commands are ignored.
  - Removed public `setTimedData(...)`; use `setIntervalMs(...)` for timed-data configuration.
- Added client connection callbacks:
  - `setClientConnectedCallback(void (*)(byte clientNo))`
  - `setClientDisconnectedCallback(void (*)(byte clientNo))`
- Added command registration API:
  - `onCommand(const char* command, void (*handler)(const char*, const char* const*, byte))`
  - `onAnyCommand(void (*handler)(const char*, const char* const*, byte))`
  - `clearAllCommandHandlers()` and `setCommandHandlerCapacity(byte)`
- **Breaking change:** Renamed public command-source client handle from `ActiveClient` to `CommandingClient`.
- Added architecture-based command parser defaults:
  - AVR: smaller defaults for command length/handler table/command name length
  - non-AVR: larger defaults (96 chars, 12 handlers, 40 command-name chars)
- Deprecated `setCommandCallback(...)` in favor of `onCommand(...)` / `onAnyCommand(...)` (legacy callback remains supported with runtime warning).

### Fixed
- Fixed timer burst issue: when the main loop is delayed beyond the timed interval, `timedWriteData` no longer fires multiple times in rapid succession to catch up. It now skips missed intervals and resumes at the next boundary.


## [5.0.2] - 2025-12-18

### Added
- New example `SineGeneratorEthernetBonjour` which uses Bonjour/mDNS service to connect using the hostname `SineNumberGenerator`

### Changed 
- Changed host name in `SineGeneratorESP32PoE`-Example from `ESP32-ETH01` to `SineGeneratorESP32_01`


## [5.0.1] - 2025-11-14

### Removed
- Removed timestamp parameter overloads from `tick()` and `tickUpdated()` methods to fix chronological ordering issues with fast timed intervals. All timestamps are now captured at transmission time to ensure proper sequential ordering.


## [5.0.0] - 2025-09-04
This is a major rewrite, not all changes are listed here.

### Added
- Added timestamp support with three modes:
  - `BLAECK_NO_TIMESTAMP` (0): No timestamp data included
  - `BLAECK_MICROS` (1): Microsecond timestamps using `micros()`
  - `BLAECK_RTC` (2): Unix epoch timestamps from RTC (requires callback)
  - `setTimestampMode(BlaeckTimestampMode mode)`
- Added RestartFlag in data messages to indicate device restart status
- Added new functions:
  - `write()` method overloads to write a single signal
  - `update()` method overloads to update a single signal
  - `tickUpdated()` and `timedWriteUpdatedData()` methods for writing only the updated signals
  - `markSignalUpdated()` and `markAllSignalsUpdated()` to mark signals as updated and `clearAllUpdateFlags()` to clear the update flags
  - `hasUpdatedSignals()` to check if any Signals are marked as updated
- Added `SignalCount` to get the number of signals added

### Changed
- **Breaking change:** On 32 bit Architecture: Data type int and unsigned int are now treated correctly as 4 byte and they use DTYPE 6 (Blaeck_long) and DTYPE 7 (Blaeck_ulong) respectively.
- **Breaking change:** `writeData()` renamed to `writeAllData()`
- **Breaking change:** `timedWriteData()` renamed to `timedWriteAllData()`
- **Breaking change:** Data message format updated from MSGKEY `B1` to `D1`
- **Breaking change:** Enhanced data transmission protocol with timestamp support, data message structure now includes: `<RestartFlag>:<TimestampMode><Timestamp>:<SymbolID><DATA><StatusByte><CRC32>`
- **Breaking change:** Deprecated old data format `<SymbolID><DATA><StatusByte><CRC32>` (used in BlaeckTCP version 4.0.1 or older)
- **Breaking change:** Renamed callback functions for improved clarity:
  - `attachUpdate()` → `setBeforeWriteCallback()`
  - `attachRead()` → `setCommandCallback()`
- Updated message parsing to handle new data format structure
- Updated the examples + some new examples added


## [4.0.1] - 2025-07-11
### Changed
- Changed buffer size defaults based on architecture

```c++
    #ifndef BLAECK_BUFFER_SIZE
      #if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
      #define BLAECK_BUFFER_SIZE 1024
      #elif defined(ARDUINO_ARCH_AVR)
      #define BLAECK_BUFFER_SIZE 32
      #else
      #define BLAECK_BUFFER_SIZE 64
      #endif
    #endif
```


## [4.0.0] - 2025-04-15
### Information
When upgrading from version 3 no changes in the sketches are required. Just update BlaeckTCP to version 4.0.0 and recompile your sketch. 

### Added
- Bridge mode (connect to a serial device over the network), see example `BridgeEthernet` or `BridgeWT32-ETH01`

### Changed
- Implemented buffered I/O with configurable buffer size (default: 1024 byte)
  - Used for reading incoming data in normal mode
  - Used for bidirectional data transfer (read/write) in bridge mode

```c++
    // Change value in BlaeckTCP.h
    #define BLAECK_BUFFER_SIZE 2048  // Default: 1024
```
- Example `BasicWT32-ETH01.ino` uses preinstalled Ethernet library (`ETH.h`)
- `WiFiS3.h` now supports server.accept() function so multiple clients can connect simultaneously


## [3.0.0] - 2024-04-16
### Information
When upgrading from version 2 no changes in the sketches are required. Just update BlaeckTCP to version 3.0.0 and recompile your sketch. 

### Changed
- **Breaking change:** Include `ServerRestarted` in response to `<BLAECK.GET_DEVICES>`, first time sending `<BLAECK.GET_DEVICES>` after a restart `ServerRestarted` is set to `1` (at other times: `0`); new message key: `MSGKEY: B5`.


## [2.2.0] - 2023-12-05

### Removed
- Removed `BlaeckTCP::addSignal(PGM_P const *signalNameTable, int signalNameIndex, ..);` because it is easier and has the same effect to use the `F()` Macro

### Changed
- Example `SignalNamesInFlashLessRAMUsage.ino` now uses the `F()` Macro to store the signal names


## [2.1.0] - 2023-12-01

### Added
- Support for the WiFiS3 library (Arduino UNO R4 WiFi) by implementation of the server.available() function (Only one client can connect simultaneously to the Arduino board)
- New example `SineGeneratorWiFiS3.ino` which uses the WiFiS3.h library

### Changed
- Removed all `Clients[i].flush()`
- Changed from `Clients[i].write('\0')` to `Clients[i].print('\0')` because `Call of overloaded function is ambiguous` error was thrown when compiling for Arduino Due and other boards


## [2.0.0] - 2023-11-03
### Information
When upgrading from 1.0.0 no changes in the sketches are required. Just update BlaeckTCP to version 2.0.0 and recompile your sketch. 

### Added
- New example `BasicETH` which uses ESP32 ETH.h library and demonstrates integration of the board ESP32-EthernetKit_A_V1.2
- New example `BasicWT32-ETH01` which uses WebServer_WT32_ETH01 library and demonstrates integration of the board WT32-ETH01 V1.4
- **Only for AVR Architecture:** Signal names can now be stored in flash memory to save RAM with the new addSignal functions `BlaeckTCP::addSignal(PGM_P const *signalNameTable, int signalNameIndex, ..);`. This is especially helpful for the ATmega328P (Arduino Uno/Nano), which only has 2048 bytes of RAM
- New example `SignalNamesInFlashLessRAMUsage.ino` added to show how it works

### Changed
- **Breaking change:** Include `Client#` and `ClientDataEnabled` in response to `<BLAECK.GET_DEVICES>`, new message key: `MSGKEY: B4`
- **Breaking change:** Behavior change of `blaeckWriteClientMask` in `BlaeckTCP::begin`; instead of every message key in 1.0.0 in the new version 2.0.0 only data (`MSGKEY: B1`) is masked and not sent to the masked clients. Devices (`MSGKEY: B4`) and symbol list `MSGKEY: B0` are always sent to all connected clients.
- **Only for ESP32:** client.flush() was removed for ESP32, because it discarded input, which led to the server not receiving commands, when the logging speed was high.
- In example SineGeneratorWiFi.ino setNoDelay is set to true per default to improve logging timing.

 
## [1.0.0] - 2023-07-28

Initial release.

[6.0.0]: https://github.com/sebaJoSt/BlaeckTCP/compare/5.0.2...6.0.0
[5.0.2]: https://github.com/sebaJoSt/BlaeckTCP/compare/5.0.1...5.0.2
[5.0.1]: https://github.com/sebaJoSt/BlaeckTCP/compare/5.0.0...5.0.1
[5.0.0]: https://github.com/sebaJoSt/BlaeckTCP/compare/4.0.1...5.0.0
[4.0.1]: https://github.com/sebaJoSt/BlaeckTCP/compare/4.0.0...4.0.1
[4.0.0]: https://github.com/sebaJoSt/BlaeckTCP/compare/3.0.0...4.0.0
[3.0.0]: https://github.com/sebaJoSt/BlaeckTCP/compare/2.2.0...3.0.0
[2.2.0]: https://github.com/sebaJoSt/BlaeckTCP/compare/2.1.0...2.2.0
[2.1.0]: https://github.com/sebaJoSt/BlaeckTCP/compare/2.0.0...2.1.0
[2.0.0]: https://github.com/sebaJoSt/BlaeckTCP/compare/1.0.0...2.0.0
[1.0.0]: https://github.com/sebaJoSt/BlaeckTCP/releases/tag/1.0.0
