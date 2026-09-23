# BlaeckTCP Support

First off, thank you for using BlaeckTCP.

Happy to help — please read the following first.

## Before asking

1. Check the [README](README.md) and [docs/network.md](docs/network.md), which cover
   connections, hosts and terminals
2. Check the [protocol documentation](https://sebajost.github.io/blaeck-protocol/)
   for anything about the frames themselves
3. Open the example nearest your problem under *File → Examples → BlaeckTCP*.
   `WaveformGenerator` exercises signals, commands, state channels and events
   together; the `Basic` examples show how each board gets online

If that did not answer it, open a
[new issue](https://github.com/sebaJoSt/BlaeckTCP/issues/new).

## When reporting a problem

Please include:

* What you expected to happen, and what happened instead
* Your board, its network hardware, and the core version
* The sketch — cut down to the smallest one that still shows the problem
* Compiler output, in full, if it does not build
* Which host is reading the data, if the sketch builds and runs but the data looks
  wrong: Loggbok, blaecktcpy, or something of your own

Two things worth attaching for anything involving missing signals, channels or
commands, because they answer most of these questions immediately:

* The output of `Blaeck.printRejections(&Serial)`, which names anything a table had
  no room for and the `begin()` call that would have made room
* What a terminal shows with `withDebugStream(&Blaeck.Terminal)`: connect with a telnet
  client such as PuTTY to see what was rejected and why, and which commands arrive
