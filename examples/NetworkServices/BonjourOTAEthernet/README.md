# Setting a Mega 2560 up for over-the-air updates

Once per board. After this the sketch can be replaced over the network, and nothing here
has to be done again. An UNO R4 needs none of it.

## Board definitions

The bootloader a Mega ships with cannot put a received sketch in place. That takes Optiboot
with `copy_flash_pages`, and the Arduino AVR package has neither that bootloader nor the fuse
settings for it. The [my_boards](https://github.com/jandrassy/my_boards) board definitions
have both.

1. Download my_boards and put it in `hardware/my_boards` inside your sketchbook folder
   (`Documents/Arduino` on Windows).
2. Restart the IDE. **Tools > Board** now lists **Arduino Mega 2560 (Optiboot)**.

## Burn the bootloader

This needs an ISP programmer - another Arduino running ArduinoISP will do, see
[how to burn a bootloader](https://arduino.stackexchange.com/questions/473/how-do-i-burn-the-bootloader).
Connect it to the ICSP header beside the ATmega2560, not the one by the USB connector.

1. Select **Arduino Mega 2560 (Optiboot)**, and your programmer under **Tools > Programmer**.
2. **Tools > Burn Bootloader**. This replaces the bootloader and erases the sketch on the board.

## Afterwards

Upload over USB with **Arduino Mega 2560 (Optiboot)** selected. The standard Mega entry talks
to the old bootloader, and its uploads fail on this one.
