#include "api_handler.h"
#include "motor_control.h"
#include "config.h"
#include <ArduinoJson.h>

WebServer* api_server_ptr = nullptr;

void setupAPIEndpoints(WebServer& server) {
    api_server_ptr = &server;
    
    // routy API
    server.on("/api", HTTP_GET, handleAPIRoot);
    server.on("/api/info", HTTP_GET, handleAPIInfo);
    server.on("/api/motor", HTTP_POST, handleAPIMotorControl);
    server.on("/api/status", HTTP_GET, handleAPIStatus);
    
    Serial.println("API endpoints configured");
}

// root API do wyświetlania dostępnych endpointów
void handleAPIRoot() {
    if (!api_server_ptr) return;
    
    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["message"] = "NeuroVehicle API";
    
    JsonObject data = doc.createNestedObject("data");
    JsonArray endpoints = data.createNestedArray("endpoints");
    
    endpoints.add("/api/info");
    endpoints.add("/api/motor");
    endpoints.add("/api/status");
    
    String response;
    serializeJson(doc, response);
    
    api_server_ptr->send(200, "application/json", response);
}

// Informacje o urządzeniu
void handleAPIInfo() {
    if (!api_server_ptr) return;
    
    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["message"] = "Device Information";
    
    JsonObject data = doc.createNestedObject("data");
    data["device"] = "NeuroVehicle";
    data["version"] = "1.0.0";
    data["ip"] = WiFi.softAPIP().toString();
    data["mac"] = WiFi.macAddress();
    data["uptime"] = millis() / 1000; // uptime w sekundach
    
    String response;
    serializeJson(doc, response);
    
    api_server_ptr->send(200, "application/json", response);
}

// Motor control via API
void handleAPIMotorControl() {
    if (!api_server_ptr) return;
    
    // sprawdzenie, czy dane są przesyłane
    if (api_server_ptr->hasArg("plain") == false) {
        StaticJsonDocument<256> errorDoc;
        errorDoc["success"] = false;
        errorDoc["message"] = "No data provided";
        
        String errorResponse;
        serializeJson(errorDoc, errorResponse);
        
        api_server_ptr->send(400, "application/json", errorResponse);
        return;
    }
    
    String data = api_server_ptr->arg("plain");
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, data);
    
    // error check
    if (error) {
        StaticJsonDocument<256> errorDoc;
        errorDoc["success"] = false;
        errorDoc["message"] = "Invalid JSON: " + String(error.c_str());
        
        String errorResponse;
        serializeJson(errorDoc, errorResponse);
        
        api_server_ptr->send(400, "application/json", errorResponse);
        return;
    }
    
    // przetwarzanie komendy
    if (doc.containsKey("command")) {
        String command = doc["command"];
        
        if (command == "forward") {
            moveForward();
        } 
        else if (command == "backward") {
            moveBackward();
        } 
        else if (command == "left") {
            turnLeft();
        } 
        else if (command == "right") {
            turnRight();
        } 
        else if (command == "stop") {
            stopMotors();
        } 
        else if (command == "speed" && doc.containsKey("left") && doc.containsKey("right")) {
            int leftSpeed = doc["left"];
            int rightSpeed = doc["right"];
            setMotorSpeed(leftSpeed, rightSpeed);
        } 
        else {
            StaticJsonDocument<256> errorDoc;
            errorDoc["success"] = false;
            errorDoc["message"] = "Unknown command or missing parameters";
            
            String errorResponse;
            serializeJson(errorDoc, errorResponse);
            
            api_server_ptr->send(400, "application/json", errorResponse);
            return;
        }
        
        // sukces
        StaticJsonDocument<256> responseDoc;
        responseDoc["success"] = true;
        responseDoc["message"] = "Command executed: " + command;
        
        String response;
        serializeJson(responseDoc, response);
        
        api_server_ptr->send(200, "application/json", response);
    } 
    else {
        // brak komendy
        StaticJsonDocument<256> errorDoc;
        errorDoc["success"] = false;
        errorDoc["message"] = "No command specified";
        
        String errorResponse;
        serializeJson(errorDoc, errorResponse);
        
        api_server_ptr->send(400, "application/json", errorResponse);
    }
}

// status pojazdu
void handleAPIStatus() {
    if (!api_server_ptr) return;
    
    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["message"] = "Vehicle Status";
    
    JsonObject data = doc.createNestedObject("data");
    
    // tutaj można dodać więcej informacji o statusie
    data["battery_voltage"] = analogRead(34) * (3.3 / 4095.0); // Przykładowe odczytanie napięcia z pinu 34
    data["free_heap"] = ESP.getFreeHeap();
    data["uptime"] = millis() / 1000; // Uptime in seconds
    
    String response;
    serializeJson(doc, response);
    
    api_server_ptr->send(200, "application/json", response);
}

// funkcje pomocnicze
String createJSONResponse(bool success, const String& message, JsonObject& data) {
    StaticJsonDocument<512> doc;
    doc["success"] = success;
    doc["message"] = message;
    doc["data"] = data;
    
    String response;
    serializeJson(doc, response);
    return response;
}