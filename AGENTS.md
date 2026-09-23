# AGENTS.md

Arduino library sending binary sensor data over TCP using the Blaeck protocol. The same
library as [BlaeckSerial](https://github.com/sebaJoSt/BlaeckSerial), over a network.

## Layout

- `src/` — the library. The only folder compiled into a sketch
  - `BlaeckCore.h/.cpp` — everything that doesn't depend on the transport. **A copy of
    BlaeckSerial's; never edit it here.** Change it in BlaeckSerial, then run
    `python extras/scripts/synccore.py`, which copies it and records the source commit in
    `extras/core-source.txt`. CI fails if the copy differs from that commit
  - `BlaeckCoreLibrary.h` — this library's namespace and settings; differs per library
  - `BlaeckTCP.h/.cpp` — the network transport: begin(), connections, hosts and
    terminals, `Blaeck.Terminal`
- `examples/` — sketches listed under *File → Examples*
- `extras/` — the doc tooling, the core sync script, and the doc-example preamble.
  Installed alongside the library, so keep it small

## Hosts and terminals

A connection becomes a host by sending a `BLAECK.` command and receives frames from then
on; every other connection is a terminal and receives only text. Hosts share one device
state. The contract is on the protocol's
[Connections page](https://sebajost.github.io/blaeck-protocol/protocol/connections);
change it there first.

## Conventions

- Call the object `Blaeck`, never `BlaeckTCP`. A variable sharing its type's
  name switches off IntelliSense for every builder chain
  ([vscode-cpptools#4251](https://github.com/microsoft/vscode-cpptools/issues/4251))
- Sources are CRLF. Check after any scripted edit
- `extras/tests/DocCodeBlocks/DocCodeBlocks.ino` is generated. It is gitignored; do not commit it
- Frame codes and byte layout belong in the
  [protocol spec](https://sebajost.github.io/blaeck-protocol/), not in the header.
  These doc comments describe what a sketch does
- Each topic example carries an identical `NetworkSetup.h`; CI fails if the copies differ

## Documenting the public API

Rules: [extras/API-STYLE.md](extras/API-STYLE.md), which points to BlaeckSerial's. Every
public name needs a doc comment and an example, and CI fails without the comment.

```
python extras/scripts/checkdocs.py src/BlaeckTCP.h src/BlaeckCore.h --skip-class BlaeckBeginRef --same-prose BlaeckTCPBeginRef=BlaeckBeginRef
python extras/scripts/checkdocs.py src/BlaeckTCP.h src/BlaeckCore.h --skip-class BlaeckBeginRef --extract
```

`--skip-class` leaves out the core's `BlaeckBeginRef`, which BlaeckTCP hides behind
`BlaeckTCPBeginRef` and whose examples call `begin(&Serial)`. `--same-prose` fails when a
wrapper's doc text drifts from the core's.

## Building

```
arduino-cli compile --fqbn arduino:avr:mega examples/BasicEthernet
```

CI compiles the examples for the Mega, the Giga, the R4 WiFi and several ESP32 boards, so
a local build is only needed to answer a specific question.

## Testing on hardware

- **As a host:** Loggbok over TCP, connected to the board's address and port
- **As a terminal:** a telnet client such as PuTTY on the same port. With
  `withDebugStream(&Blaeck.Terminal)` it shows connections, received commands and what the
  library refused. A terminal never receives a frame; if one shows binary, it has become a
  host by sending a `BLAECK.` command

## Releasing

The two registries are triggered by different things, so a version number is not
just a label:

- **PlatformIO** watches the repository and publishes as soon as the `version` in
  `library.properties` changes on the branch. No tag, no `pio` install, nothing
  manual. Whatever is on the branch that day becomes a release for real users.
- **Arduino Library Manager** publishes from a git tag, and ignores the version
  field until then.

So bump the version as the last step before tagging, never while developing. 7.0.0
already reached PlatformIO mid-development and cannot be reused.

**The crawler can be switched off, once.** Publishing a single version by hand ends
auto-crawling permanently. Do it from the local folder before pushing:

```
pio account login
# bump both manifests locally, do not push yet
pio pkg publish
# then push and tag — publishing is manual from here on
```

Published versions are listed at
[registry.platformio.org/libraries/sebajost/BlaeckTCP](https://registry.platformio.org/libraries/sebajost/BlaeckTCP).

## Related

[BlaeckSerial](https://github.com/sebaJoSt/BlaeckSerial) is the same library over a
serial port and sends byte-identical frames. It is the reference: the core, the API and
the examples' Blaeck parts come from there.

<!-- CLAUDE.md is a one-line @AGENTS.md import, so Claude Code reads this file too.
     Keep the content here; that file exists only as a bridge. -->
