# An Open OBDII API for a Smarter Car
Open OBD API on a TinyZero

This project aims to provide a universal platform in the age of IoT through which users can make data requests for their cars supporting OBDII. Our device will plug into the OBDII slot of supported vehicles and beam information via bluetooth. The command
interface for our device will aim to make creating applications on bluetooth enabled devices simple and easy.

We are using this Diagnostic Code Table as a reference:
http://www.uobdii.com/upload/service/diagnostic-trouble-code-table.pdf

Our platform of choice in implementing our API is the TinyZero, running on a Cortex-M0:
https://tinycircuits.com/collections/kits-1/products/tinyzero-basic-kit

A Bluetooth module compatible with the TinyZero is also available:
https://tinycircuits.com/collections/communication/products/bluetooth-low-energy-tinyshield

Finally, during development, we will focus more on the software than hardware design. We will be using the Freematics OBD UART adapter:
https://freematics.com/pages/products/freematics-obd-ii-uart-adapter-mk2/

## Libraries
Bluetooth: https://github.com/TinyCircuits/TinyCircuits-TinyShield-BLE-ASD2116/tree/master/examples/STBLE
OBD Adapter: https://github.com/stanleyhuangyc/ArduinoOBD/tree/master/libraries/OBD2UART

## Schedule for Module
1. Design a connector circuit that adheres to voltage and current draw specification. (1 week)
  * Design circuit diagram
  * Implement in hardware
2. Interface with vehicle and perform raw data read-out over UART (1 week)
  * Write basic program that can run OBDII pins at proper modulation scheme
  * Attempt to send data commands
3. Attach bluetooth capabilities for UART over Bluetooth (1 week)
  * Write or introduce driver for SPI interface
  * Initialize the bluetooth stack for both RX and TX
  * Liaison UART data through bluetooth


## Schedule for Firmware
1. Use OBD-2 specification to gradually decode generic diagnostic info
2. Design an application API in the form of a UART command interface to make data requests in real time (1 week)
3. Design command format/packet frame
4. Repackage requests and responses in packet format
