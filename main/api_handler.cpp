#include "api_handler.h"
#include "motor_control.h"
#include "config.h"
#include <ArduinoJson.h>
#include <WiFi.h>

WebServer* api_server_ptr = nullptr;

void setupAPIEndpoints(WebServer& server) {
    api_server_ptr = &server;
    
    // routy API
    server.on("/api", HTTP_GET, handleAPIRoot);
    server.on("/api/info", HTTP_GET, handleAPIInfo);
    server.on("/api/motor", HTTP_POST, handleAPIMotorControl);
    server.on("/api/status", HTTP_GET, handleAPIStatus);
    server.on("/api/docs", HTTP_GET, handleAPIDocs);
    
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

// Add this new handler function
void handleAPIDocs() {
    if (!api_server_ptr) return;
    
    String html = "<!DOCTYPE html>";
    html += "<html><head><title>NeuroVehicle API Documentation</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; line-height: 1.6; }";
    html += "h1 { color: #333; }";
    html += "h2 { color: #444; margin-top: 30px; }";
    html += ".endpoint { background-color: #f5f5f5; padding: 15px; border-radius: 5px; margin-bottom: 20px; }";
    html += ".method { display: inline-block; padding: 5px 10px; border-radius: 3px; color: white; font-weight: bold; margin-right: 10px; }";
    html += ".get { background-color: #61affe; }";
    html += ".post { background-color: #49cc90; }";
    html += "pre { background-color: #272822; color: #f8f8f2; padding: 10px; border-radius: 4px; overflow-x: auto; }";
    html += ".curl-form { margin-top: 20px; background-color: #e8f0fe; padding: 15px; border-radius: 5px; }";
    html += "input, select { margin: 10px 0; padding: 8px; width: 100%; box-sizing: border-box; }";
    html += "button { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; border-radius: 5px; cursor: pointer; }";
    html += ".result { margin-top: 15px; font-family: monospace; }";
    html += "</style></head>";
    html += "<body>";
    html += "<h1>NeuroVehicle API Documentation</h1>";
    
    // API Overview
    html += "<p>This API allows you to control the NeuroVehicle and retrieve status information.</p>";
    
    // GET /api endpoint
    html += "<div class='endpoint'>";
    html += "<h2><span class='method get'>GET</span> /api</h2>";
    html += "<p>Returns a list of all available API endpoints.</p>";
    html += "<h3>Response Example:</h3>";
    html += "<pre>{<br>  \"success\": true,<br>  \"message\": \"NeuroVehicle API\",<br>  \"data\": {<br>    \"endpoints\": [<br>      \"/api/info\",<br>      \"/api/motor\",<br>      \"/api/status\"<br>    ]<br>  }<br>}</pre>";
    html += "</div>";
    
    // GET /api/info endpoint
    html += "<div class='endpoint'>";
    html += "<h2><span class='method get'>GET</span> /api/info</h2>";
    html += "<p>Returns information about the device.</p>";
    html += "<h3>Response Example:</h3>";
    html += "<pre>{<br>  \"success\": true,<br>  \"message\": \"Device Information\",<br>  \"data\": {<br>    \"device\": \"NeuroVehicle\",<br>    \"version\": \"1.0.0\",<br>    \"ip\": \"192.168.4.1\",<br>    \"mac\": \"XX:XX:XX:XX:XX:XX\",<br>    \"uptime\": 1234<br>  }<br>}</pre>";
    html += "</div>";
    
    // POST /api/motor endpoint
    html += "<div class='endpoint'>";
    html += "<h2><span class='method post'>POST</span> /api/motor</h2>";
    html += "<p>Controls the vehicle motors.</p>";
    html += "<h3>Request Format:</h3>";
    html += "<pre>{<br>  \"command\": \"forward|backward|left|right|stop|speed\",<br>  \"left\": 100,   // Only for 'speed' command<br>  \"right\": 100   // Only for 'speed' command<br>}</pre>";
    html += "<h3>Response Example:</h3>";
    html += "<pre>{<br>  \"success\": true,<br>  \"message\": \"Command executed: forward\"<br>}</pre>";
    
    // Curl Generator
    html += "<div class='curl-form'>";
    html += "<h3>Generate curl Command:</h3>";
    html += "<select id='command'>";
    html += "<option value='forward'>forward</option>";
    html += "<option value='backward'>backward</option>";
    html += "<option value='left'>left</option>";
    html += "<option value='right'>right</option>";
    html += "<option value='stop'>stop</option>";
    html += "<option value='speed'>speed</option>";
    html += "</select>";
    
    html += "<div id='speed-params' style='display: none;'>";
    html += "<label for='left-speed'>Left Speed (-255 to 255):</label>";
    html += "<input type='number' id='left-speed' min='-255' max='255' value='100'>";
    html += "<label for='right-speed'>Right Speed (-255 to 255):</label>";
    html += "<input type='number' id='right-speed' min='-255' max='255' value='100'>";
    html += "</div>";
    
    html += "<button onclick='generateCurl()'>Generate curl Command</button>";
    html += "<div class='result'><pre id='curl-output'></pre></div>";
    html += "</div>";
    html += "</div>";
    
    // GET /api/status endpoint
    html += "<div class='endpoint'>";
    html += "<h2><span class='method get'>GET</span> /api/status</h2>";
    html += "<p>Returns the current status of the vehicle.</p>";
    html += "<h3>Response Example:</h3>";
    html += "<pre>{<br>  \"success\": true,<br>  \"message\": \"Vehicle Status\",<br>  \"data\": {<br>    \"battery_voltage\": 3.7,<br>    \"free_heap\": 123456,<br>    \"uptime\": 1234<br>  }<br>}</pre>";
    html += "</div>";
    
    // JavaScript for the curl generator
    html += "<script>";
    html += "document.getElementById('command').addEventListener('change', function() {";
    html += "  const speedParams = document.getElementById('speed-params');";
    html += "  speedParams.style.display = this.value === 'speed' ? 'block' : 'none';";
    html += "});";
    
    html += "function generateCurl() {";
    html += "  const command = document.getElementById('command').value;";
    html += "  let jsonData = {command: command};";
    
    html += "  if (command === 'speed') {";
    html += "    jsonData.left = parseInt(document.getElementById('left-speed').value);";
    html += "    jsonData.right = parseInt(document.getElementById('right-speed').value);";
    html += "  }";
    
    html += "  const jsonString = JSON.stringify(jsonData, null, 2);";
    html += "  const serverAddress = window.location.hostname;";
    html += "  const curlCommand = `curl -X POST http://${serverAddress}/api/motor \\
    -H \"Content-Type: application/json\" \\
    -d '${jsonString}'`;";
    
    html += "  document.getElementById('curl-output').textContent = curlCommand;";
    html += "}";
    html += "</script>";
    
    html += "</body></html>";
    
    api_server_ptr->send(200, "text/html", html);
}