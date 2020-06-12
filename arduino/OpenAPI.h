#ifndef OPEN_API_H
#define OPEN_API_H

#include <Stream.h>
#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_STD_STREAM 0
#include "ArduinoJson.h"
//#include "OBD2UART.h"

/* The API class that interprets commands, and replies with data.
 * It requires Stream access to a client, for API requests.
 * The class depends on the ArduinoJson library (https://arduinojson.org/)
 * for its easily portable JSON handlers.
 */
class OpenAPI
{
public:

    /* Instantiates an API handler.
     * This should also start the OBD adapter for reading.
     */
    OpenAPI();

    /* Processes incoming data for requests, and generates
     * replies back to the client by querying the OBD adapter.
     * This should be called often.
     */
    void process(Stream& clientStream);

private:
    StaticJsonDocument<BUFSIZ> jsonData;
    char _buffer[BUFSIZ];
};

#endif /* OPEN_API_H */
