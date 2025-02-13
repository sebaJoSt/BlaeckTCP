<a href="url"><img src="https://user-images.githubusercontent.com/388152/185908831-4eccf7a6-5f43-405d-b7fe-5225eeba302d.png" height="75"></a>
<a href="url"><img src="https://github.com/sebaJoSt/BlaeckTCP/assets/388152/15f6a932-2263-4453-9686-0ad9e36720fd"  alt="BlaeckTCP Logo SeeSaw Font" height="70"></a>
===



BlaeckTCP is a simple Arduino library to send binary (sensor) data via Ethernet/WiFi to your PC. The data can be sent periodically or requested on demand with [commands](#blaecktcp-commands).
Also included is a message parser which reads input in the syntax of `<HelloWorld, 12, 47>`. The parsed command `HelloWorld` and its parameters are available in your own sketch by attaching a callback function.


## Getting Started

Clone this repository into `Arduino/Libraries` or use the built-in Arduino IDE Library manager to install
a copy of this library. You can find more detail about installing libraries 
[here, on Arduino's website](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-installing-a-library).

(Open BasicEthernet Example under `File -> Examples -> BlaeckTCP` for reference)

```CPP
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <BlaeckTCP.h>
```
### Instantiate BlaeckTCP
```CPP
BlaeckTCP BlaeckTCP;
```
### Initialize Serial & BlaeckTCP
```CPP
void setup()
{
  Serial.begin(9600);

   BlaeckTCP.begin(
      MAX_CLIENTS, // Maximal number of allowed clients
      &Serial,     // Serial reference, used for debugging
      2,           // Maximal signal count used;
      0b11111101   // Clients allowed to receive Blaeck Data; from right to left: client #0, #1, .. , #7
  );
}
```

### Add signals
```CPP
BlaeckTCP.addSignal("Small Number", &randomSmallNumber);
BlaeckTCP.addSignal("Big Number", &randomBigNumber);
```

### Start TCP-Server and listen for clients
```CPP
TelnetPrint = NetServer(SERVER_PORT);
TelnetPrint.begin();
```

### Update your variables and don't forget to `tick()`!
```CPP
void loop()
{
  UpdateYourVariables();

 /*Keeps watching for commands from TCP client and
     transmits the data back to client at the user-set interval*/
  BlaeckTCP.tick();
}
```

## BlaeckTCP commands

Here's a full list of the commands handled by this library:

| Command                      | Description                                                                      |
| ---------------------------- | -------------------------------------------------------------------------------- |
| `<BLAECK.GET_DEVICES>`       | Writes the device information including the device name, hardware version, firmware version and library version |
| `<BLAECK.WRITE_SYMBOLS> `    | Writes symbol list including datatype information.                               |
| `<BLAECK.WRITE_DATA> `       | Writes the binary data.                                                          |
| `<BLAECK.ACTIVATE,first,second,third,fourth byte>`| Activates writing the binary data in user-set interval [ms]<br />Min: 0ms  Max: 4294967295ms<br /> e.g. `<BLAECK.ACTIVATE,96,234>` The data is written every 60 seconds (60 000ms)<br />first Byte: 0b01100000 = 96 DEC<br />second Byte: 0b11101010 = 234 DEC|
| `<BLAECK.DEACTIVATE> `       | Deactivates writing in intervals.                                                |

## Messages

The messages are in the following format:
````
|Header|--       Message        --||-- EOT  --|
<BLAECK:<MSGKEY>:<MSGID>:<ELEMENTS>/BLAECK>\r\n
````

Type| MSGKEY | Elements| Description
----|--------|---------------------------------|---------------------------------------------
Symbol List | B0 | **`<MasterSlaveConfig><SlaveID><SymbolName><DTYPE>`** | **Up to n symbols.** Response to request for available symbols `<BLAECK.WRITE_SYMBOLS>`
Data | B1 | **`<SymbolID><DATA>`**`<StatusByte><CRC32>` | **Up to n data items.** Response to request for data `<BLAECK.WRITE_DATA>`
~~Devices~~ | ~~B3~~ | ~~`<MasterSlaveConfig><SlaveID><DeviceName><DeviceHWVersion><DeviceFWVersion><LibraryVersion><LibraryName>`~~ | Deprecated (Used in BlaeckTCP v1)
~~Devices~~ | ~~B4~~ | ~~`<MasterSlaveConfig><SlaveID><DeviceName><DeviceHWVersion><DeviceFWVersion><LibraryVersion><LibraryName><Client#><ClientDataEnabled>`~~ | Deprecated (Used in BlaeckTCP v2)
Devices | B5 | `<MasterSlaveConfig><SlaveID><DeviceName><DeviceHWVersion><DeviceFWVersion><LibraryVersion><LibraryName><Client#><ClientDataEnabled><ServerRestarted>` | Only one device (No master/slave support). Response to request for device information `<BLAECK.GET_DEVICES>`
  

 Element|Type    |  DESCRIPTION:
 -------|--------| ---------------------------------------------------------------------------
 `MSGKEY`| byte |  Message Key, A unique key for the type of message being sent; 1 byte transmitted
 `MSGID` | ulong|  Message ID,  A unique message ID which echoes back to transmitter to indicate a response to a message (0 to 4294967295); 4 bytes transmitted
 `DATA`  | (varying)| Message Data, varying data types and length depending on message
 `SymbolID` | uint | Symbol ID number
 `SymbolName` |String0 | Symbol Name - Null Terminated String
 `DTYPE` | byte | DataType  0=bool, 1=byte, 2=short, 3=ushort, 4=int, 5=uint, 6=long, 7=ulong, 8=float
  `MasterSlaveConfig`     | byte | always 0: Single device (no master/slave support in this library)
   `SlaveID`              | byte |             Slave Address, always 0 (no master/slave support in this library)
   `DeviceName`           | String0 |          set with public variable `DeviceName`
   `DeviceHWVersion`      | String0 |          set with public variable `DeviceHWVersion`
   `DeviceFWVersion`      | String0 |          set with public variable `DeviceFWVersion`
   `LibraryVersion`       | String0 |          set with public const `LIBRARY_VERSION`
   `LibraryName`          | String0 |          set with public const `LIBRARY_NAME`
   `Client#`              | String0 |          Client number of the connected client
   `ClientDataEnabled`    | String0 |          0 or 1; Client is not allowed/allowed to receive Data (`MSGKEY`: `B1`); Set with `blaeckWriteDataClientMask` in `BlaeckTCP::begin`
   `ServerRestarted`      | String0 |          0 or 1;  first time sending `<BLAECK.GET_DEVICES>` after a restart `ServerRestarted` is set to 1 (at other times: 0)
   `StatusByte`           | byte |             1 byte; Always 0: Normal Transmission (no master/slave support in this library)
   `CRC32`                | byte |             4 bytes; CRC order: 32; CRC Polynom (hex): 4C11DB7; Initial value (hex): FFFFFFFF; Final XOR value (hex): FFFFFFFF; reverse data bytes: true; reverse CRC result before Final XOR: true; (http://zorc.breitbandkatze.de/crc.html)
         
   
 
 MSGID is supported by `<BLAECK.GET_DEVICES>`, `<BLAECK.WRITE_SYMBOLS>` and `<BLAECK.WRITE_DATA>`:
 ````
 <BLAECK.GET_DEVICES, firstByteMSGID, secondByteMSGID, thirdByteMSGID, fourthByteMSGID>
 <BLAECK.WRITE_SYMBOLS, firstByteMSGID, secondByteMSGID, thirdByteMSGID, fourthByteMSGID>
 <BLAECK.WRITE_DATA, firstByteMSGID, secondByteMSGID, thirdByteMSGID, fourthByteMSGID>
 ````
 
 ## Decoding Examples
 
 ### Symbol List Decoding
 Example from `BasicEthernet.ino`:
 `<BLAECK.WRITE_SYMBOLS, 0, 255, 0, 0>`:
 ````
ASCII: <  B  L  A  E  C  K  :  °  :  °  °  °  °  :  °  °  S  m  a  l  l     N  u  m  b
HEX:   3C 42 4C 41 45 43 4B 3A B0 3A 00 FF 00 00 3A 00 00 53 6D 61 6C 6C 20 4E 75 6D 62
Byte:  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26
-----------------------------------------------------------------------------------
ASCII: e  r  °  °  °  °  B  i  g     N  u  m  b  e  r  °  °  /  B  L  A  E  C  K  >
HEX:   65 72 00 08 00 00 42 69 67 20 4E 75 6D 62 65 72 00 06 2F 42 4C 41 45 43 4B 3E
Byte:  27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 56 47 48 49 50 51 52
 ````
 
 Byte | DESCRIPTION:
----|---------------------------------------------
8   | `MSGKEY`: B0 -> Symbol List
10-13| `MSGID`: Hex: 00 FF 00 00 -> Decimal: 65280
15  | `MasterSlaveConfig`: Hex: 00 -> Decimal: 0 -> Single device
16  | `SlaveID`: Hex: 00 -> Decimal: 0 -> Slave Address: 0
17-29| `SymbolName`: ASCII: Small Number + Null Termination '\0'
30   | `DTYPE`: Hex: 08 -> Float
31  | `MasterSlaveConfig`: Hex: 00 -> Decimal: 0 -> Single device
32  | `SlaveID`: Hex: 00 -> Decimal: 0 -> Slave Address: 0
33-43| `SymbolName`: ASCII: Big Number + Null Termination '\0'
44   | `DTYPE`: Hex: 06 -> Long

 ### Data Decoding
 Example from `BasicEthernet.ino`:
 `<BLAECK.WRITE_DATA, 255, 255, 255, 255>`:
 ````
ASCII: <  B  L  A  E  C  K  :  °  :  °  °  °  °  :  °  °  °  °  °  °  °  °  °  °  °  °  °  °  °  °  °  /  B  L  A  E  C  K  >  \r \n
HEX:   3C 42 4C 41 45 43 4B 3A B1 3A FF FF FF FF 3A 00 00 B8 1E FD 40 01 00 D8 E6 32 7C 00 FE D9 3D 20 2F 42 4C 41 45 43 4B 3E 0D 0A
Byte:  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41
 ````
 
 Byte | DESCRIPTION:
----|---------------------------------------------
8   | `MSGKEY`: B1 -> Data
10-13| `MSGID`: Hex: FF FF FF FF -> Decimal: 4294967295
15-16| `SymbolID`: Hex: 00 00 -> Decimal: 0
17-20| `DATA`: Float -> 4 Bytes; Hex: B8 1E FD 40 -> Float: 7.91
21-22| `SymbolID`: Hex: 01 00 -> Decimal: 1
23-26| `DATA`: Long  -> 4 Bytes; Hex: D8 E6 32 7C -> Long: 2083710680
27   | `StatusByte`: 0 -> Normal Transmission
28-31| `CRC32`: 4 Bytes; Hex: 20 3D D9 FE (Calculated from 19 bytes: Byte 8-26)

## Datatypes

`DTYPE` | Datatype | Bytes
-- |----|---------------------------------------------
0| bool | 1
1|byte | 1
2|short| 2
3|unsigned short| 2
4|int| 2
5|unsigned int | 2
6|long | 4
7|unsigned long | 4
8|float | 4
9|double | 8

On the Uno and other ATMEGA based boards, the double implementation occupies 4 byte and is exactly the same as the float, with no gain in precision. Therefore if you add a double signal with `addSignal("My Double Signal", &doubleVariable)` the symbol list will return the `DTYPE` 8 (float).
