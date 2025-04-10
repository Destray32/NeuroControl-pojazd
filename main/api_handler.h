#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <WebServer.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>

void setupAPIEndpoints(WebServer& server);
void setupWebSocketServer(WebSocketsServer& ws_server);
void handleSensorWebSocket();

// handlery API
void handleAPIRoot();
void handleAPIInfo();
void handleAPIMotorControl();
void handleAPIStatus();
void handleAPIDocs();
void handleAPIMode();
void handleAPISensor();

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
void processCommand(const String& command, const JsonDocument& doc);

// funkcje pomocnicze
String createJSONResponse(bool success, const String& message, JsonObject& data);

#endif // API_HANDLER_H