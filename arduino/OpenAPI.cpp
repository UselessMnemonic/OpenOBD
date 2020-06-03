#include "OpenAPI.h"
#include <sam.h>

OpenAPI::OpenAPI() {
    // wheeeeeee
}

void OpenAPI::process(Stream& clientStream) {

    // process only when data is available
    if (!clientStream.available()) return;

    // TODO: not impossible that this may hang, so implement a watchdog.
    DeserializationError result = deserializeJson(jsonData, clientStream);

    // TODO: uniquely handle each result case
    if (result == DeserializationError::IncompleteInput)
        return;
        
    if (result != DeserializationError::Ok) {
        printf("BLE: Got bad JSON data. Recovery may be delayed.");
        jsonData.clear();
        jsonData["error"] = "Bad JSON";
    }
    else {
      SerialUSB.print("-- Got Request --\r\n");
      serializeJsonPretty(jsonData, SerialUSB);
      SerialUSB.print("\r\n-- End Request --\r\n");
      
      // Parse for PID request
      if (jsonData.containsKey("pid")) {
        jsonData["result"] = 100;
      }
      else {
        serializeJson(jsonData, _buffer);
        jsonData.clear();
        jsonData["error"] = "Bad Request";
      }
    }

    SerialUSB.print("-- Sending Reply --\r\n");
    serializeJsonPretty(jsonData, SerialUSB);
    SerialUSB.print("\r\n-- End Reply --\r\n");
    
    serializeJson(jsonData, _buffer);
    clientStream.write(_buffer, strlen(_buffer));

    jsonData.clear();
    jsonData.garbageCollect();
}
