#include <sam.h>
#include "bluetooth.h"
#include <SPI.h>
#include <stdio.h>

#include "UartStream.h"
#include "OpenAPI.h"

// libc functions
extern "C" void __libc_init_array(void);
extern "C" int _write(int fd, const void *buf, size_t count) {
  if (fd == 1) SerialUSB.write((char*) buf, count);
  return 0;
}

// UART stream and BLE hook
UartStream bleUART;
void bluetooth_on_uart_rx(uint8_t* buf, uint8_t len) {
    bleUART.notify(buf, len);
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

  // API handle
  OpenAPI apiHandle;

  // main loop
  bluetooth_enable();
  while (1) {
    bluetooth_handle();
    apiHandle.process(bleUART);
  }
}
