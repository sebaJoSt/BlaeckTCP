# Setting a board up for over-the-air updates

WaveformGenerator turns on updates over the network with `#define NETWORK_WITH_SERVICES`,
and any other example can do the same. Before that works, each board needs a one-time setup.
After it, the sketch can be replaced over the network, and nothing here has to be done again.

The board listens for an upload on port 65280, with the password `password`, and announces
itself with Bonjour under the sketch's `HOST_NAME`, along with its BlaeckTCP port.

## Arduino Mega 2560

### Board definitions

The bootloader a Mega ships with cannot put a received sketch in place. That takes Optiboot
with `copy_flash_pages`, and the Arduino AVR package has neither that bootloader nor the fuse
settings for it. The [my_boards](https://github.com/jandrassy/my_boards) board definitions
have both.

1. Download my_boards and put it in `hardware/my_boards` inside your sketchbook folder
   (`Documents/Arduino` on Windows).
2. Restart the IDE. **Tools > Board** now lists **Arduino Mega 2560 (Optiboot)**.

### Burn the bootloader

This needs an ISP programmer - another Arduino running ArduinoISP will do, see
[how to burn a bootloader](https://arduino.stackexchange.com/questions/473/how-do-i-burn-the-bootloader).
Connect it to the ICSP header beside the ATmega2560, not the one by the USB connector.

1. Select **Arduino Mega 2560 (Optiboot)**, and your programmer under **Tools > Programmer**.
2. **Tools > Burn Bootloader**. This replaces the bootloader and erases the sketch on the board.

### Afterwards

Upload over USB with **Arduino Mega 2560 (Optiboot)** selected. The standard Mega entry talks
to the old bootloader, and its uploads fail on this one.

## Arduino Giga R1

`NetworkSetup.h` includes the QSPI storage adapter when Giga network services are enabled.
No extra sketch header is needed. Install the **Arduino_Portenta_OTA** library as well.

### Partition the QSPI flash

An update is kept on the QSPI flash, which is unpartitioned when the board is new. The
bootloader looks for the update on its second partition.

1. Open **File > Examples > STM32H747_System > QSPIFormat** and upload it.
2. Open the serial monitor at 115200 baud.
3. Answer **Y** to creating the four partitions.
4. A full erase is not needed on a board that has never been partitioned.
5. Say **Y** to restoring the WiFi firmware and certificates if you use WiFi on this board.
   Nothing here needs them - the sketch arrives over Ethernet - but formatting clears the
   partition they live on.

It ends with `QSPI Flash formatted!`.

### Bootloader

Version 22 or newer applies an update from the QSPI. Boards have shipped with it for a long
time; on an older one, an upload stops with a message on the serial port.
**File > Examples > STM32H747_System > STM32H747_manageBootloader** updates it.

## ESP32-PoE and WT32-ETH01

Once per computer, and again after every ESP32 core update.

### Which ArduinoOTA

Two libraries carry that name, and they work in opposite directions.

The one bundled with the ESP32 core has the board fetch the sketch: the PC invites the board
and then waits for the board to connect back to it. A firewall on the PC blocks that
connection, and on a locked-down machine there is no way around it.

The one in the Library Manager, the same one the Ethernet shield boards use, has the board
listen on port 65280 and the PC push the sketch to it. Only an outgoing connection from the
PC is needed, which firewalls allow.

These examples use the second one. A sketch that includes `ArduinoOTA.h` on an ESP32 always
gets the bundled one, so it has to be removed.

### Remove the bundled library

1. Install **ArduinoOTA** from the Library Manager.
2. Delete the folder `ArduinoOTA` under the ESP32 core's `libraries`:
   `%LOCALAPPDATA%\Arduino15\packages\esp32\hardware\esp32\<version>\libraries\ArduinoOTA`
   on Windows, `~/.arduino15/...` on Linux and `~/Library/Arduino15/...` on macOS.
3. Restart the IDE.

Updating or reinstalling the ESP32 core puts the folder back, and the example stops
compiling until it is removed again. The compiler then complains about `ArduinoOTA.begin`
taking no arguments, which is the bundled library's `begin`.

### What you give up

The Arduino IDE's own network upload no longer reaches this board: for an ESP32 it always
speaks to the bundled library's protocol. Upload over USB, or over the network with a tool
that pushes to port 65280.

## Arduino UNO R4

Needs none of this. It isn't one of the boards the topic examples cover.
