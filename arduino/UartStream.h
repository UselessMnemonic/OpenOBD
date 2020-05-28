#ifndef UART_STREAM_H
#define UART_STREAM_H

#include <Arduino.h>

class UartStream : public Stream
{
public:
    UartStream() : writePos(0), readPos(0), length(0) {}
    int available();
    int read();
    int peek();
    size_t write(const uint8_t* buffer, size_t size);
    size_t write(uint8_t b);
    void flush();
    void notify(uint8_t* buf, int len);
private:
    uint8_t buffer[BUFSIZ];
    int writePos;
    int readPos;
    int length;
};

#endif /* UART_STREAM_H */