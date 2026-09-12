# Setting a Giga R1 up for over-the-air updates

Once per board. After this the sketch can be replaced over the network, and nothing here
has to be done again.

## Libraries

Install `EthernetBonjour`, `ArduinoOTA` (by Juraj Andrassy) and `Arduino_Portenta_OTA`.

## Partition the QSPI flash

A Giga has no room in its own flash for a second copy of a sketch, so an update is kept on
the 16 MB QSPI chip beside the processor. That chip is unpartitioned when the board is new,
and the bootloader looks for the update on its second partition.

1. Open **File > Examples > STM32H747_System > QSPIFormat** and upload it.
2. Open the serial monitor at 115200 baud.
3. Answer **Y** to creating the four partitions.
4. A full erase is not needed on a board that has never been partitioned.
5. Say **Y** to restoring the WiFi firmware and certificates if you use WiFi on this board.
   Nothing here needs them - the sketch arrives over Ethernet - but formatting clears the
   partition they live on.

It ends with `QSPI Flash formatted!`.

## Bootloader

Version 22 or newer applies an update from the QSPI. Boards have shipped with it for a long
time, and the sketch says so on the serial port if this one has not. **File > Examples >
STM32H747_System > STM32H747_manageBootloader** updates it.

## Uploading

Send the sketch to the board's address or to `BonjourOTAEthernetGiga.local`, with the
password in `ArduinoOTA.begin()`. An update may be up to 5 MB, the size of the partition -
unlike the boards that keep one in their own flash, where half of it is the limit.
