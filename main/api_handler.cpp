#include "api_handler.h"
#include "motor_control.h"
#include "sensor_control.h"
#include "config.h"
#include <ArduinoJson.h>
#include <WiFi.h>

unsigned long lastSensorTime = 0;
const unsigned long SENSOR_INTERVAL = 100;

enum ControlMode
{
    WEB_CONTROL,
    ML_CONTROL
};

enum SpeedMode
{
    SPEED_LOW,
    SPEED_NORMAL,
    SPEED_HIGH
};

ControlMode currentMode = WEB_CONTROL;     // domyślnie kontrola to manualna przez serwer web
SpeedMode currentSpeedMode = SPEED_NORMAL; // domyślny tryb prędkości to normalny
WebSocketsServer *wsServer = nullptr;
WebServer *api_server_ptr = nullptr;

void addCORSHeaders()
{
    if (!api_server_ptr)
        return;
    api_server_ptr->sendHeader("Access-Control-Allow-Origin", "*");
    api_server_ptr->sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    api_server_ptr->sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    api_server_ptr->sendHeader("Access-Control-Max-Age", "86400"); // 24 hours cache
}

void handleOptions()
{
    if (!api_server_ptr)
        return;

    addCORSHeaders();
    api_server_ptr->send(200, "text/plain", "OK");
}

// Funkcja do ustawienia aktualnej prędkości silników na podstawie wybranego trybu
int getCurrentSpeedValue()
{
    switch (currentSpeedMode)
    {
    case SPEED_LOW:
        return SPEED_MODE_LOW;
    case SPEED_HIGH:
        return SPEED_MODE_HIGH;
    case SPEED_NORMAL:
    default:
        return SPEED_MODE_NORMAL;
    }
}

// Konwersja trybu prędkości na string
String getSpeedModeString()
{
    switch (currentSpeedMode)
    {
    case SPEED_LOW:
        return "low";
    case SPEED_HIGH:
        return "high";
    case SPEED_NORMAL:
    default:
        return "normal";
    }
}

// Konwersja stringa na tryb prędkości
bool setSpeedModeFromString(const String &mode)
{
    if (mode == "low")
    {
        currentSpeedMode = SPEED_LOW;
        return true;
    }
    else if (mode == "normal")
    {
        currentSpeedMode = SPEED_NORMAL;
        return true;
    }
    else if (mode == "high")
    {
        currentSpeedMode = SPEED_HIGH;
        return true;
    }
    return false;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_ERROR:
        Serial.printf("WebSocket %u ERROR\n", num);
        break;
    case WStype_DISCONNECTED:
        Serial.printf("WebSocket %u rozlaczony\n", num);
        break;
    case WStype_CONNECTED:
    {
        IPAddress ip = wsServer->remoteIP(num);
        Serial.printf("WebSocket %u polaczony z %s\n", num, ip.toString().c_str());

        wsServer->sendTXT(num, "{\"status\":\"connected\",\"mode\":" + String(currentMode == WEB_CONTROL ? "\"web\"" : "\"ml\"") + "}");
    }
    break;
    case WStype_TEXT:
    {
        Serial.printf("WebSocket %u odebrano: %s\n", num, payload);

        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
            Serial.printf("Bład deserializacji JSON: %s\n", error.c_str());
            wsServer->sendTXT(num, "{\"success\": false, \"message\": \"Bład deserializacji JSON\"}");
            return;
        }

        if (doc.containsKey("mode"))
        {
            String mode = doc["mode"];
            if (mode == "web")
            {
                currentMode = WEB_CONTROL;
                wsServer->broadcastTXT("{\"status\":\"mode_changed\",\"mode\":\"web\"}");
                Serial.println("Kontrola zmieniona na kontrole WEB");
            }
            else if (mode == "ml")
            {
                currentMode = ML_CONTROL;
                wsServer->broadcastTXT("{\"status\":\"mode_changed\",\"mode\":\"ml\"}");
                Serial.println("Kontrola zmieniona na kontrole ML");
            }
        }

        // interpretacja komendy zależnie od akutalnego trybu sterowania
        if (doc.containsKey("command"))
        {
            String command = doc["command"];

            // jesli w ML trybie to tylko kontrole ML zezwalamy
            if (currentMode == ML_CONTROL && doc.containsKey("source") && doc["source"] == "ml")
            {
                processCommand(command, doc);
            }
            // vice versa
            else if (currentMode == WEB_CONTROL && doc.containsKey("source") && doc["source"] == "web")
            {
                processCommand(command, doc);
            }
            else
            {
                Serial.println("Komenda odrzucona z powodu zlego argumentu 'source'");
                wsServer->sendTXT(num, "{\"status\":\"error\",\"message\":\"Komenda odrzucona z powodu zlego argumentu 'source'\"}");
            }
        }
    }
    break;
    }
}

// Process movement commands from either source
void processCommand(const String &command, const JsonDocument &doc)
{
    if (command == "forward")
    {
        moveForward();
        if (wsServer)
            wsServer->broadcastTXT("{\"status\":\"moving\",\"direction\":\"forward\"}");
    }
    else if (command == "backward")
    {
        moveBackward();
        if (wsServer)
            wsServer->broadcastTXT("{\"status\":\"moving\",\"direction\":\"backward\"}");
    }
    else if (command == "left")
    {
        turnLeft();
        if (wsServer)
            wsServer->broadcastTXT("{\"status\":\"moving\",\"direction\":\"left\"}");
    }
    else if (command == "right")
    {
        turnRight();
        if (wsServer)
            wsServer->broadcastTXT("{\"status\":\"moving\",\"direction\":\"right\"}");
    }
    else if (command == "stop")
    {
        stopMotors();
        if (wsServer)
            wsServer->broadcastTXT("{\"status\":\"stopped\"}");
    }
    else if (command == "speed" && doc.containsKey("left") && doc.containsKey("right"))
    {
        int leftSpeed = doc["left"];
        int rightSpeed = doc["right"];
        setMotorSpeed(leftSpeed, rightSpeed);
        if (wsServer)
        {
            String response = "{\"status\":\"speed_set\",\"left\":" + String(leftSpeed) + ",\"right\":" + String(rightSpeed) + "}";

            wsServer->broadcastTXT(response);
        }
    }
    else if (command == "set_speed_mode" && doc.containsKey("mode"))
    {
        String mode = doc["mode"];
        bool success = setSpeedModeFromString(mode);
        if (success)
        {
            if (wsServer)
            {
                String response = "{\"status\":\"speed_mode_changed\",\"mode\":\"" + mode + "\"}";
                wsServer->broadcastTXT(response);
            }
            Serial.println("Speed mode changed to: " + mode);
        }
        else
        {
            if (wsServer)
            {
                wsServer->broadcastTXT("{\"status\":\"error\",\"message\":\"Invalid speed mode. Use 'low', 'normal', or 'high'\"}");
            }
            Serial.println("Invalid speed mode: " + mode);
        }
    }
}

void setupAPIEndpoints(WebServer &server)
{
    api_server_ptr = &server;

    // requesty OPTIONS dla wszystkich endpointów
    server.on("/api", HTTP_OPTIONS, handleOptions);
    server.on("/api/info", HTTP_OPTIONS, handleOptions);
    server.on("/api/motor", HTTP_OPTIONS, handleOptions);
    server.on("/api/status", HTTP_OPTIONS, handleOptions);
    server.on("/api/docs", HTTP_OPTIONS, handleOptions);
    server.on("/api/mode", HTTP_OPTIONS, handleOptions);
    server.on("/api/sensor", HTTP_OPTIONS, handleOptions);

    // routy API
    server.on("/api", HTTP_GET, handleAPIRoot);
    server.on("/api/info", HTTP_GET, handleAPIInfo);
    server.on("/api/motor", HTTP_ANY, handleAPIMotorControl);
    server.on("/api/status", HTTP_GET, handleAPIStatus);
    server.on("/api/docs", HTTP_GET, handleAPIDocs);
    server.on("/api/mode", HTTP_ANY, handleAPIMode);
    server.on("/api/sensor", HTTP_ANY, handleAPISensor); // endpoint do obsługi sensora HC-SR04

    Serial.println("API endpoints configured");
}

// root API do wyświetlania dostępnych endpointów
void handleAPIRoot()
{
    if (!api_server_ptr)
        return;

    addCORSHeaders();

    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["message"] = "NeuroVehicle API";

    JsonObject data = doc.createNestedObject("data");
    JsonArray endpoints = data.createNestedArray("endpoints");

    endpoints.add("/api/info");
    endpoints.add("/api/motor");
    endpoints.add("/api/status");
    endpoints.add("/api/docs");
    endpoints.add("/api/mode");
    endpoints.add("/api/sensor");
    // endpointy kamery i strumienia
    endpoints.add("/capture");
    endpoints.add("/stream");
    endpoints.add("/camera/resolution");

    String response;
    serializeJson(doc, response);

    api_server_ptr->send(200, "application/json", response);
}

// Informacje o urządzeniu
void handleAPIInfo()
{
    if (!api_server_ptr)
        return;

    addCORSHeaders();

    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["message"] = "Device Information";

    JsonObject data = doc.createNestedObject("data");
    data["device"] = "NeuroVehicle";
    data["version"] = "3.0.0";
    data["ip"] = WiFi.softAPIP().toString();
    data["mac"] = WiFi.softAPmacAddress();
    data["uptime"] = millis() / 1000; // uptime w sekundach

    String response;
    serializeJson(doc, response);

    api_server_ptr->send(200, "application/json", response);
}

// Motor control via API
void handleAPIMotorControl()
{
    if (!api_server_ptr)
        return;

    addCORSHeaders();

    // Dla metody GET zwracamy aktualny stan silników i tryb prędkości
    if (api_server_ptr->method() == HTTP_GET)
    {
        StaticJsonDocument<256> doc;
        doc["success"] = true;
        doc["message"] = "Motor status";

        JsonObject data = doc.createNestedObject("data");
        data["speed_mode"] = getSpeedModeString();

        String response;
        serializeJson(doc, response);

        api_server_ptr->send(200, "application/json", response);
        return;
    }

    // sprawdzenie, czy dane są przesyłane
    if (api_server_ptr->hasArg("plain") == false)
    {
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
    if (error)
    {
        StaticJsonDocument<256> errorDoc;
        errorDoc["success"] = false;
        errorDoc["message"] = "Invalid JSON: " + String(error.c_str());

        String errorResponse;
        serializeJson(errorDoc, errorResponse);

        api_server_ptr->send(400, "application/json", errorResponse);
        return;
    }

    // przetwarzanie komendy
    if (doc.containsKey("command"))
    {
        String command = doc["command"];

        if (command == "forward")
        {
            moveForward();
        }
        else if (command == "backward")
        {
            moveBackward();
        }
        else if (command == "left")
        {
            turnLeft();
        }
        else if (command == "right")
        {
            turnRight();
        }
        else if (command == "stop")
        {
            stopMotors();
        }
        else if (command == "speed" && doc.containsKey("left") && doc.containsKey("right"))
        {
            int leftSpeed = doc["left"];
            int rightSpeed = doc["right"];
            setMotorSpeed(leftSpeed, rightSpeed);
        }
        else if (command == "set_speed_mode" && doc.containsKey("mode"))
        {
            String mode = doc["mode"];
            bool success = setSpeedModeFromString(mode);

            if (success)
            {
                StaticJsonDocument<256> responseDoc;
                responseDoc["success"] = true;
                responseDoc["message"] = "Speed mode set to: " + mode;

                String response;
                serializeJson(responseDoc, response);

                api_server_ptr->send(200, "application/json", response);
                return;
            }
            else
            {
                StaticJsonDocument<256> errorDoc;
                errorDoc["success"] = false;
                errorDoc["message"] = "Invalid speed mode. Use 'low', 'normal', or 'high'";

                String errorResponse;
                serializeJson(errorDoc, errorResponse);

                api_server_ptr->send(400, "application/json", errorResponse);
                return;
            }
        }
        else
        {
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
    else
    {
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
void handleAPIStatus()
{
    if (!api_server_ptr)
        return;

    addCORSHeaders();

    StaticJsonDocument<512> doc;
    doc["success"] = true;
    doc["message"] = "Vehicle Status";

    JsonObject data = doc.createNestedObject("data");

    // tutaj można dodać więcej informacji o statusie
    data["battery_voltage"] = analogRead(34) * (3.3 / 4095.0); // Przykładowe odczytanie napięcia z pinu 34
    data["free_heap"] = ESP.getFreeHeap();
    data["uptime"] = millis() / 1000;                // Uptime in seconds
    data["signal_strength"] = WiFi.RSSI();           // Siła sygnału WiFi
    data["motor_speed_mode"] = getSpeedModeString(); // Aktualny tryb prędkości
    data["control_mode"] = (currentMode == WEB_CONTROL) ? "web" : "ml";

    String response;
    serializeJson(doc, response);

    api_server_ptr->send(200, "application/json", response);
}

// funkcje pomocnicze
String createJSONResponse(bool success, const String &message, JsonObject &data)
{
    StaticJsonDocument<512> doc;
    doc["success"] = success;
    doc["message"] = message;
    doc["data"] = data;

    String response;
    serializeJson(doc, response);
    return response;
}

// Add this new handler function
void handleAPIDocs()
{
    if (!api_server_ptr)
        return;

    addCORSHeaders();

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
    html += "<pre>{<br>  \"success\": true,<br>  \"message\": \"NeuroVehicle API\",<br>  \"data\": {<br>    \"endpoints\": [<br>      \"/api/info\",<br>      \"/api/motor\",<br>      \"/api/status\",<br>      \"/api/docs\",<br>      \"/api/mode\",<br>      \"/api/sensor\"<br>    ]<br>  }<br>}</pre>";
    html += "</div>";

    // GET /api/info endpoint
    html += "<div class='endpoint'>";
    html += "<h2><span class='method get'>GET</span> /api/info</h2>";
    html += "<p>Returns information about the device.</p>";
    html += "<h3>Response Example:</h3>";
    html += "<pre>{<br>  \"success\": true,<br>  \"message\": \"Device Information\",<br>  \"data\": {<br>    \"device\": \"NeuroVehicle\",<br>    \"version\": \"3.0.0\",<br>    \"ip\": \"192.168.4.1\",<br>    \"mac\": \"XX:XX:XX:XX:XX:XX\",<br>    \"uptime\": 1234<br>  }<br>}</pre>";
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
    html += "<pre>{<br>  \"success\": true,<br>  \"message\": \"Vehicle Status\",<br>  \"data\": {<br>    \"battery_voltage\": 3.7,<br>    \"free_heap\": 123456,<br>    \"uptime\": 1234,<br>    \"signal_strength\": -70,<br>    \"motor_speed_mode\": \"normal\",<br>    \"control_mode\": \"web\"<br>  }<br>}</pre>";
    html += "</div>";

    // NEW: GET /api/sensor endpoint
    html += "<div class='endpoint'>";
    html += "<h2><span class='method get'>GET</span> /api/sensor</h2>";
    html += "<p>Returns the current distance reading from the HC-SR04 ultrasonic sensor.</p>";
    html += "<p>The vehicle uses this sensor for obstacle detection during forward movement.</p>";
    html += "<h3>Response Example (Success):</h3>";
    html += "<pre>{<br>  \"data\": {<br>    \"distance\": 42.5,<br>    \"success\": true,<br>    \"message\": \"Distance retrieved successfully\"<br>  }<br>}</pre>";
    html += "<h3>Response Example (Error):</h3>";
    html += "<pre>{<br>  \"data\": {<br>    \"distance\": -1,<br>    \"success\": false,<br>    \"message\": \"Failed to retrieve distance\"<br>  }<br>}</pre>";
    html += "<div class='curl-form'>";
    html += "<h3>Get Current Distance:</h3>";
    html += "<button onclick='getSensorReading()'>Get Distance</button>";
    html += "<div class='result'><pre id='sensor-output'>Click to get the current distance reading</pre></div>";
    html += "</div>";
    html += "</div>";

    // WebSocket info
    html += "<div class='endpoint'>";
    html += "<h2>WebSocket Interface</h2>";
    html += "<p>The vehicle provides a WebSocket interface on port 82 for real-time control.</p>";
    html += "<h3>Connection:</h3>";
    html += "<pre>ws://" + WiFi.softAPIP().toString() + ":82</pre>";
    html += "<h3>Message Format:</h3>";
    html += "<pre>{<br>  \"source\": \"web|ml\",  // Source of the command<br>  \"command\": \"forward|backward|left|right|stop|speed\",<br>  \"left\": 100,   // Only for 'speed' command<br>  \"right\": 100   // Only for 'speed' command<br>}</pre>";
    html += "<h3>Mode Selection:</h3>";
    html += "<pre>{<br>  \"mode\": \"web|ml\"  // Switch between web control and ML control<br>}</pre>";
    html += "<h3>Sensor Data:</h3>";
    html += "<p>The WebSocket also broadcasts sensor data periodically:</p>";
    html += "<pre>{<br>  \"status\": \"success\",<br>  \"distance\": 42.5,<br>  \"timestamp\": 1234567890<br>}</pre>";
    html += "</div>";

    // Add mode control form
    html += "<div class='endpoint'>";
    html += "<h2><span class='method post'>POST</span> /api/mode</h2>";
    html += "<p>Changes the control mode between web interface and ML application.</p>";
    html += "<h3>Request Format:</h3>";
    html += "<pre>{<br>  \"mode\": \"web|ml\"<br>}</pre>";
    html += "<h3>Response Example:</h3>";
    html += "<pre>{<br>  \"success\": true,<br>  \"message\": \"Mode changed to web control\"<br>}</pre>";
    html += "<div class='curl-form'>";
    html += "<h3>Set Control Mode:</h3>";
    html += "<select id='control-mode'>";
    html += "<option value='web'>Web Control</option>";
    html += "<option value='ml'>ML Control</option>";
    html += "</select>";
    html += "<button onclick='setMode()'>Set Mode</button>";
    html += "<div class='result'><pre id='mode-output'></pre></div>";
    html += "</div>";
    html += "</div>";

    // JavaScript for all interactive elements
    html += "<script>";
    // Existing code for command generator
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
    html += "  const curlCommand = `curl -X POST http://${serverAddress}/api/motor \\\\";
    html += "    -H \"Content-Type: application/json\" \\\\";
    html += "    -d '${jsonString}'`;";

    html += "  document.getElementById('curl-output').textContent = curlCommand;";
    html += "}";

    // Mode setter function
    html += "function setMode() {";
    html += "  const mode = document.getElementById('control-mode').value;";
    html += "  const jsonData = {mode: mode};";
    html += "  const jsonString = JSON.stringify(jsonData);";
    html += "  const serverAddress = window.location.hostname;";
    html += "  fetch(`http://${serverAddress}/api/mode`, {";
    html += "    method: 'POST',";
    html += "    headers: {";
    html += "      'Content-Type': 'application/json'";
    html += "    },";
    html += "    body: jsonString";
    html += "  })";
    html += "  .then(response => response.json())";
    html += "  .then(data => {";
    html += "    document.getElementById('mode-output').textContent = JSON.stringify(data, null, 2);";
    html += "  })";
    html += "  .catch(error => {";
    html += "    document.getElementById('mode-output').textContent = 'Error: ' + error;";
    html += "  });";
    html += "}";

    // New sensor reading function
    html += "function getSensorReading() {";
    html += "  const serverAddress = window.location.hostname;";
    html += "  document.getElementById('sensor-output').textContent = 'Loading...';";
    html += "  fetch(`http://${serverAddress}/api/sensor`)";
    html += "  .then(response => response.json())";
    html += "  .then(data => {";
    html += "    document.getElementById('sensor-output').textContent = JSON.stringify(data, null, 2);";
    html += "  })";
    html += "  .catch(error => {";
    html += "    document.getElementById('sensor-output').textContent = 'Error: ' + error;";
    html += "  });";
    html += "}";

    // Camera image capture function
    html += "function captureImage() {";
    html += "  const serverAddress = window.location.hostname;";
    html += "  const imgContainer = document.getElementById('camera-image');";
    html += "  imgContainer.innerHTML = 'Loading...';";
    html += "  ";
    html += "  const img = new Image();";
    html += "  img.onload = function() {";
    html += "    imgContainer.innerHTML = '';";
    html += "    img.style.width = '100%';";
    html += "    img.style.maxWidth = '640px';";
    html += "    imgContainer.appendChild(img);";
    html += "  };";
    html += "  img.onerror = function() {";
    html += "    imgContainer.innerHTML = 'Failed to load image';";
    html += "  };";
    html += "  img.src = `http://${serverAddress}/capture?t=${new Date().getTime()}`;";
    html += "}";

    // Camera stream toggle function
    html += "let streamActive = false;";
    html += "function toggleStream() {";
    html += "  const serverAddress = window.location.hostname;";
    html += "  const streamContainer = document.getElementById('stream-container');";
    html += "  const streamImg = document.getElementById('stream');";
    html += "  ";
    html += "  if (streamActive) {";
    html += "    streamContainer.style.display = 'none';";
    html += "    streamImg.src = '';";
    html += "    streamActive = false;";
    html += "  } else {";
    html += "    streamContainer.style.display = 'block';";
    html += "    streamImg.src = `http://${serverAddress}/stream`;";
    html += "    streamActive = true;";
    html += "  }";
    html += "}";

    // Camera resolution setter function
    html += "function setResolution() {";
    html += "  const resolution = document.getElementById('camera-resolution').value;";
    html += "  const serverAddress = window.location.hostname;";
    html += "  const outputElem = document.getElementById('resolution-output');";
    html += "  outputElem.textContent = 'Setting resolution...';";
    html += "  ";
    html += "  fetch(`http://${serverAddress}/camera/resolution?res=${resolution}`)";
    html += "  .then(response => response.text())";
    html += "  .then(data => {";
    html += "    outputElem.textContent = data;";
    html += "    // If we have an active stream, refresh it to show the new resolution";
    html += "    if (streamActive) {";
    html += "      const streamImg = document.getElementById('stream');";
    html += "      streamImg.src = `http://${serverAddress}/stream?t=${new Date().getTime()}`;";
    html += "    }";
    html += "  })";
    html += "  .catch(error => {";
    html += "    outputElem.textContent = 'Error: ' + error;";
    html += "  });";
    html += "}";

    html += "</script>";

    // Camera API documentation
    html += "<div class='endpoint'>";
    html += "<h2>Camera API</h2>";
    html += "<p>The NeuroVehicle includes a camera that can be accessed via these endpoints.</p>";

    // GET /capture endpoint
    html += "<div class='endpoint'>";
    html += "<h2><span class='method get'>GET</span> /capture</h2>";
    html += "<p>Captures a single JPEG image from the camera.</p>";
    html += "<h3>Response:</h3>";
    html += "<p>Returns a JPEG image with Content-Type: image/jpeg</p>";
    html += "<div class='curl-form'>";
    html += "<h3>View Camera Image:</h3>";
    html += "<button onclick='captureImage()'>Capture Image</button>";
    html += "<div id='camera-image' class='result' style='margin-top: 15px;'></div>";
    html += "</div>";
    html += "</div>";

    // GET /stream endpoint
    html += "<div class='endpoint'>";
    html += "<h2><span class='method get'>GET</span> /stream</h2>";
    html += "<p>Starts an MJPEG stream from the camera.</p>";
    html += "<h3>Response:</h3>";
    html += "<p>Returns a multipart/x-mixed-replace stream of JPEG images</p>";
    html += "<div class='curl-form'>";
    html += "<h3>View Camera Stream:</h3>";
    html += "<button onclick='toggleStream()'>Start/Stop Stream</button>";
    html += "<div class='result' style='margin-top: 15px;'>";
    html += "<div id='stream-container' style='display:none;'>";
    html += "<img id='stream' src='' style='width:100%;max-width:640px;'>";
    html += "</div>";
    html += "</div>";
    html += "</div>";
    html += "</div>";

    // GET /camera/resolution endpoint
    html += "<div class='endpoint'>";
    html += "<h2><span class='method get'>GET</span> /camera/resolution</h2>";
    html += "<p>Changes the camera resolution.</p>";
    html += "<h3>Parameters:</h3>";
    html += "<code>?res=RESOLUTION</code> where RESOLUTION is one of: UXGA, SXGA, XGA, SVGA, VGA, CIF, QVGA</p>";
    html += "<h3>Response Example:</h3>";
    html += "<pre>Resolution set to SVGA</pre>";
    html += "<div class='curl-form'>";
    html += "<h3>Set Camera Resolution:</h3>";
    html += "<select id='camera-resolution'>";
    html += "<option value='UXGA'>UXGA (1600x1200)</option>";
    html += "<option value='SXGA'>SXGA (1280x1024)</option>";
    html += "<option value='XGA'>XGA (1024x768)</option>";
    html += "<option value='SVGA' selected>SVGA (800x600)</option>";
    html += "<option value='VGA'>VGA (640x480)</option>";
    html += "<option value='CIF'>CIF (400x296)</option>";
    html += "<option value='QVGA'>QVGA (320x240)</option>";
    html += "</select>";
    html += "<button onclick='setResolution()'>Set Resolution</button>";
    html += "<div class='result'><pre id='resolution-output'></pre></div>";
    html += "</div>";
    html += "</div>";

    html += "</div>"; // Close camera API main div

    html += "</body></html>";

    api_server_ptr->send(200, "text/html", html);
}

void handleAPIMode()
{
    if (!api_server_ptr)
        return;

    addCORSHeaders();

    if (api_server_ptr->method() == HTTP_GET)
    {
        // Return current mode
        StaticJsonDocument<256> doc;
        doc["success"] = true;
        doc["message"] = "Current control mode";

        JsonObject data = doc.createNestedObject("data");
        data["mode"] = currentMode == WEB_CONTROL ? "web" : "ml";

        String response;
        serializeJson(doc, response);

        api_server_ptr->send(200, "application/json", response);
    }
    else if (api_server_ptr->method() == HTTP_POST)
    {
        // Change mode
        if (api_server_ptr->hasArg("plain") == false)
        {
            api_server_ptr->send(400, "application/json", "{\"success\":false,\"message\":\"No data provided\"}");
            return;
        }

        String data = api_server_ptr->arg("plain");
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, data);

        if (error)
        {
            api_server_ptr->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }

        if (doc.containsKey("mode"))
        {
            String mode = doc["mode"];
            if (mode == "web")
            {
                currentMode = WEB_CONTROL;
                if (wsServer)
                    wsServer->broadcastTXT("{\"status\":\"mode_changed\",\"mode\":\"web\"}");
                api_server_ptr->send(200, "application/json", "{\"success\":true,\"message\":\"Mode changed to web control\"}");
            }
            else if (mode == "ml")
            {
                currentMode = ML_CONTROL;
                if (wsServer)
                    wsServer->broadcastTXT("{\"status\":\"mode_changed\",\"mode\":\"ml\"}");
                api_server_ptr->send(200, "application/json", "{\"success\":true,\"message\":\"Mode changed to ML control\"}");
            }
            else
            {
                api_server_ptr->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid mode. Use 'web' or 'ml'\"}");
            }
        }
        else
        {
            api_server_ptr->send(400, "application/json", "{\"success\":false,\"message\":\"No mode specified\"}");
        }
    }
}

void handleAPISensor()
{
    if (!api_server_ptr)
        return;

    addCORSHeaders();

    // Dla metody GET zwracamy aktualny stan sensora
    if (api_server_ptr->method() == HTTP_GET)
    {
        StaticJsonDocument<256> doc;

        JsonObject data = doc.createNestedObject("data");
        data["distance"] = getDistance(); // funkcja do odczytu live danych z sensora
        data["success"] = (data["distance"] != -1);
        data["message"] = (data["success"]) ? "Distance retrieved successfully" : "Failed to retrieve distance";

        String response;
        serializeJson(doc, response);

        if (data["success"])
        {
            api_server_ptr->send(200, "application/json", response);
        }
        else
        {
            api_server_ptr->send(500, "application/json", response);
        }
        return;
    }
}

void handleSensorWebSocket()
{
    if (!wsServer || wsServer->connectedClients() == 0)
        return;

    unsigned long currentTime = millis();
    if (currentTime - lastSensorTime >= SENSOR_INTERVAL)
    {
        float distance = getDistance();

        DynamicJsonDocument doc(128);
        doc["status"] = (distance != -1) ? "success" : "error";
        doc["distance"] = (distance != -1) ? distance : 0; // zwraca dystans albo 0 w przypadku błędu
        doc["timestamp"] = currentTime;                    // znacznik czasu

        String message;
        serializeJson(doc, message);

        wsServer->broadcastTXT(message);
        lastSensorTime = currentTime;
    }
}

void setupWebSocketServer(WebSocketsServer &ws_server)
{
    wsServer = &ws_server;

    // Set up header validation to allow any origin
    wsServer->onValidateHttpHeader([](String headerName, String headerValue)
                                   {
        if (headerName.equalsIgnoreCase("Origin")) {
            // Accept any origin
            Serial.printf("WebSocket connection from origin: %s\n", headerValue.c_str());
            return true;
        }
        // Accept all other headers
        return true; }, nullptr, 0);

    wsServer->onEvent(webSocketEvent);
    Serial.println("Serwer WebSocket przypisany z obsługą CORS");
}