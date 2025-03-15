#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <WebServer.h>
#include <ArduinoJson.h>

void setupAPIEndpoints(WebServer& server);

// handlery API
void handleAPIRoot();
void handleAPIInfo();
void handleAPIMotorControl();
void handleAPIStatus();

// funkcje pomocnicze
String createJSONResponse(bool success, const String& message, JsonObject& data);

#endif // API_HANDLER_H