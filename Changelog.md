# Changelog

All notable changes to this project will be documented in this file.

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

[2.1.0]: https://github.com/sebaJoSt/BlaeckTCP/compare/2.0.0...2.1.0
[2.0.0]: https://github.com/sebaJoSt/BlaeckTCP/compare/1.0.0...2.0.0
[1.0.0]: https://github.com/sebaJoSt/BlaeckTCP/releases/tag/1.0.0
