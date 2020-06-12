#ifndef UART_STREAM_H
#define UART_STREAM_H

#include <Stream.h>

/* This class allows us to pass UART data using the Stream interface.
 * It buffers input from notify(), which must be called from bluetooth_on_uart_rx().
 * This way, the stream is populated any time data is recieved. It is then
 * buffered into a cyclic buffer, from which it may be read.
 * 
 * Big thanks to https://github.com/SofaPirate/StringStream for a reference cyclic buffer
 * and Stream.
 */
class UartStream : public Stream
{
public:
    /* Instantiate an empty Stream */
    UartStream() : writePos(0), readPos(0), length(0) {}

    /* Returns however many bytes are available to read out of the stream. */
    int available();

    /* Returns and removes the next character in the stream,
     * or -1 if the stream is empty.
     */
    int read();

    /* Returns, but does not remove, the next character in the
     * stream, or -1 if the stream is empty.
     */
    int peek();

    /* Write up to size characters into the stream.
     * Returns the number of bytes sent via Bluetooth, or
     * 0 if an error occured during transmission.
     */
    size_t write(const uint8_t* buffer, size_t size);

    /* Write a single character into the stream.
     * Returns 1 if successful, 0 otherwise.
     */
    size_t write(uint8_t b);

    /* Inert. */
    void flush();

    /* Notifies the stream of new data.
     * buf is taken from bluetooth_on_uart_rx().
     * May be useful as a way of manually invoking commands.
     */
    void notify(uint8_t* buf, int len);

private:
    uint8_t buffer[BUFSIZ];
    int writePos;
    int readPos;
    int length;
};

#endif /* UART_STREAM_H */
