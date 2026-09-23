# Configuration

The table sizes and the switches that remove features are shared with BlaeckSerial and
described in its [configuration guide](https://github.com/sebaJoSt/BlaeckSerial/blob/master/docs/configuration.md).
This page covers what only BlaeckTCP has.

## Where settings go

A setting is a `#define` that has to reach both your sketch and the library's `.cpp` files.
If only one of them sees it, memory is corrupted without an error.

- **PlatformIO:** `build_flags = -DBLAECK_TCP_NO_DELAY_DEFAULT=false`
- **Arduino IDE:** a `BlaeckTCPConfig.h` on the compiler's include path. The sketch folder
  is not on it; BlaeckSerial's guide lists the ways to get it there, which work the same way
  for this file.

## Buffered writes

`BLAECK_BUFFERED_WRITES_DEFAULT` is `true` on every board. Each frame is built in RAM and
sent with one write per host. Unbuffered, an Ethernet shield turns every small write into a
TCP send of its own, so a frame leaves in many small packets.

The buffer is sized from the signals added and grows if a frame needs more. On an AVR board
short of RAM, turn it off with `setBufferedWrites(false)` or
`#define BLAECK_BUFFERED_WRITES_DEFAULT false`.

## Nagle's algorithm

`BLAECK_TCP_NO_DELAY_DEFAULT` is `true`: on ESP32 and ESP8266, small packets are sent at once
rather than held back to be combined. Set it `false` to favour throughput over latency. Other
boards ignore it.

## Connections

The number of connections is set in the sketch, with `withClients()` on the `begin()` chain;
see [Network](network.md).
