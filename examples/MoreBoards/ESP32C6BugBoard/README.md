# ESP32-C6-Bug with the Ethernet add-on

The sketch runs on the ESP32-C6-Bug (V2.1.0) with the ESP32-BUG-ETH add-on (V1.0.0), whose
W5500 Ethernet chip sits on SPI.

## Setup

1. Install Espressif's ESP32 core, if it isn't already.
2. Select **ESP32C6 Dev Module**. The Bug board has no board definition of its own.
3. Upload the sketch. The W5500's pins are set at the top of it.

## Why it isn't in NetworkSetup.h

Every board selected as ESP32C6 Dev Module looks the same to a sketch, so NetworkSetup.h
couldn't tell a Bug board from any other C6 and would assume the add-on's pins on all of
them. This sketch sets them itself instead.
