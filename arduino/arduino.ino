#include <sam.h>
#include "bluetooth.h"
#include <SPI.h>
#include <stdio.h>
#include "UartStream.h"

// libc functions
extern "C" void __libc_init_array(void);
extern "C" int _write(int fd, const void *buf, size_t count) {
  if (fd == 1) SerialUSB.write((char*) buf, count);
  return 0;
}

// UART stream and BLE hook
UartStream BleUART;
void bluetooth_on_uart_rx(uint8_t* buf, uint8_t len) {
    BleUART.notify(buf, len);
}

// burn, baby! burn!
int main(void) {

  // device initialization
  init();
  __libc_init_array();
  __enable_irq();
  USBDevice.init();
  USBDevice.attach();
  while (!USBDevice.connected()) {}

  // bluetooth initialization
  bluetooth_init();
  bluetooth_enable();

  // for tests
  char buffer[BUFSIZ];
  int available;

  // main loop
  while (1) {
    bluetooth_handle();

    available = BleUART.available();
    if (available > 0) {
      BleUART.readBytes(buffer, (size_t)available);
      buffer[available] = 0;
      printf("BLE: Got [%d]: %s\r\n", available, buffer);
    }

    available = SerialUSB.available();
    if (available > 0) {
      SerialUSB.readBytes(buffer, (size_t)available);
      buffer[available] = 0;
      printf("BLE: Sent [%d]: %s\r\n", available, buffer);
      BleUART.write((const uint8_t*)buffer, (size_t)available);
    }

  }
}
