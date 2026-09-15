# Setting a Giga R1 up for over-the-air updates

Once per board. After this the sketch can be replaced over the network, and nothing here
has to be done again.

## Partition the QSPI flash

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

## Bootloader

Version 22 or newer applies an update from the QSPI. Boards have shipped with it for a long
time; on an older one, an upload stops with a message on the serial port.
**File > Examples > STM32H747_System > STM32H747_manageBootloader** updates it.
