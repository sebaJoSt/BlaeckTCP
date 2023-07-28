[![arduino-library-badge](https://www.ardu-badge.com/badge/BlaeckTCP.svg)](https://www.ardu-badge.com/BlaeckTCP)

<a href="url"><img src="https://user-images.githubusercontent.com/388152/185908831-4eccf7a6-5f43-405d-b7fe-5225eeba302d.png" height="75"></a>
<a href="url"><img src="https://github.com/sebaJoSt/BlaeckTCP/assets/388152/15f6a932-2263-4453-9686-0ad9e36720fd"  alt="BlaeckTCP Logo SeeSaw Font" height="70"></a>
===



BlaeckTCP is a simple Arduino library to send binary (sensor) data via Ethernet/WiFi to your PC. The data can be sent periodically or requested on demand with [commands](##BlaeckTCP-Commands).
Also included is a message parser which reads input in the syntax of `<HelloWorld, 12, 47>`. The parsed command `HelloWorld` and its parameters are available in your own sketch by attaching a callback function.

BlaeckTCP is inspired by Nick Dodd's [AdvancedSerial Library](https://github.com/Nick1787/AdvancedSerial/).
The message parser uses code from Robin2's Arduino forum thread [Serial Basic Input](https://forum.arduino.cc/index.php?topic=396450.0).
