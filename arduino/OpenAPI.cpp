#include "OpenAPI.h"

OpenAPI::OpenAPI() {
    // wheeeeeee
}

void OpenAPI::process(Stream& clientStream) {

    // process only when data is available
    if (!clientStream.available()) return;

    // TODO: not impossible that this may hang, so implement a watchdog.
    DeserializationError result = deserializeJson(jsonData, clientStream);

    // TODO: uniquely handle each result case
    if (result != DeserializationError::Ok) {
        jsonData.garbageCollect();
        jsonData["error"] = "Bad JSON";
    }
    else {
      // Parse for PID request
      if (!jsonData.containsKey("pid")) return;

      // Find data, then reply with JSON
      jsonData["pid"] = 100;
    }

    serializeJson(jsonData, _buffer);
    clientStream.write(_buffer, strlen(_buffer));
    jsonData.garbageCollect();
}