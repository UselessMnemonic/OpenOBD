#include <sam.h>
#include "bluetooth.h"
#include <SPI.h>
#include <stdio.h>

// libc functions
extern "C" void __libc_init_array(void);
extern "C" int _write(int fd, const void *buf, size_t count) {
  if (fd == 1) SerialUSB.write((char*) buf, count);
  return 0;
}

// event handlers
void bluetooth_on_uart_rx(uint8_t *buf, uint8_t len) {
  printf("BLE: Got [%d]: %s\r\n", len, (const char*)buf);
  //oobd_process_command(...)
}

// input buffer
char input_buffer[1024];

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

  // main loop
  while (1) {
    bluetooth_handle();
    int num_bytes_available = SerialUSB.available();
    if (num_bytes_available > 0) {
      SerialUSB.readBytes(input_buffer, num_bytes_available);
      input_buffer[num_bytes_available] = '\0';
      bluetooth_uart_tx((uint8_t*) input_buffer, (uint32_t) num_bytes_available);
      printf("BLE: Sent [%d]: %s", num_bytes_available, input_buffer);
    }
  }
}
