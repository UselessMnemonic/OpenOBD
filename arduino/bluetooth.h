#ifndef BLUETOOTH
#define BLUETOOTH

/**
 * Initializes the bluetooth module
 */
void bluetooth_init();

/**
 * Handles any ACI commands/events from the bluetooh devices
 */
void bluetooth_handle();

/**
 * Performs a UART transmission to any connected devices
 * 
 * @param buf  The buffer form which data is read
 * @param len  The length of the data buffer, in bytes
 * 
 * @return     1 if the transmission was successful, 0 otherwise
 */
uint8_t bluetooth_uart_tx(uint8_t *buf, uint32_t len);

/**
 * Callback when a UART transmission is pending from a connected device
 * 
 * @param buf  The buffer to which data is saved
 * @param len  The length of the data buffer, in bytes
 */
extern void bluetooth_on_uart_rx(uint8_t *buf, uint8_t len);

/**
 * @return 1 if the bluetooth device is connected, 0 otherwise
 */
uint8_t bluetooth_connected();

/**
 * Enables the bluetooth module to be used by a host
 */
void bluetooth_enable();

/**
 * Prevents the bluetooth module from being used by a host
 */
void bluetooth_disable();

#endif /* BLUETOOTH_H */
