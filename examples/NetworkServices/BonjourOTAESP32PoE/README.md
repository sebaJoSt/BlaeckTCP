# Setting an ESP32 up for over-the-air updates

Once per computer, and again after every ESP32 core update. After this the sketch can be
replaced over the network.

## Which ArduinoOTA

Two libraries carry that name, and they work in opposite directions.

The one bundled with the ESP32 core has the board fetch the sketch: the PC invites the board
and then waits for the board to connect back to it. A firewall on the PC blocks that
connection, and on a locked-down machine there is no way around it.

The one in the Library Manager, the same one the Ethernet shield examples use, has the board
listen on port 65280 and the PC push the sketch to it. Only an outgoing connection from the
PC is needed, which firewalls allow.

This example uses the second one. A sketch that includes `ArduinoOTA.h` on an ESP32 always
gets the bundled one, so it has to be removed.

## Remove the bundled library

1. Install **ArduinoOTA** from the Library Manager.
2. Delete the folder `ArduinoOTA` under the ESP32 core's `libraries`:
   `%LOCALAPPDATA%\Arduino15\packages\esp32\hardware\esp32\<version>\libraries\ArduinoOTA`
   on Windows, `~/.arduino15/...` on Linux and `~/Library/Arduino15/...` on macOS.
3. Restart the IDE.

Updating or reinstalling the ESP32 core puts the folder back, and the example stops
compiling until it is removed again. The compiler then complains about `ArduinoOTA.begin`
taking no arguments, which is the bundled library's `begin`.

## What you give up

The Arduino IDE's own network upload no longer reaches this board: for an ESP32 it always
speaks to the bundled library's protocol. Upload over USB, or over the network with a tool
that pushes to port 65280.
