#include "UartStream.h"
#include "bluetooth.h"

int UartStream::available() {
    return length;
}

int UartStream::read() {
    int val = peek();
    if (val >= 0) {
        readPos = (readPos + 1) % BUFSIZ; 
        length--;
    }
    return val;
}

int UartStream::peek() {
    return (available() == 0 ? (-1) : (int)(buffer[readPos]));
}

size_t UartStream::write(const uint8_t* buffer, size_t size) {
    uint8_t ret = bluetooth_uart_tx((uint8_t*)buffer, (uint32_t)size);
    if (ret == 1) return size;
    return 0;
}

size_t UartStream::write(uint8_t b) {
    return bluetooth_uart_tx(&b, 1);
}

void UartStream::notify(uint8_t* buf, int len) {
    if (length >= BUFSIZ) return;

    for (int i = 0; i < len && length < BUFSIZ; i++) {
        buffer[writePos] = buf[i];
        writePos = (writePos + 1) % BUFSIZ;
        length++;
    }
}

void UartStream::flush() {}