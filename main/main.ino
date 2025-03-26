#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include "config.h"
#include "motor_control.h"
#include "web_server.h"
#include "api_handler.h"

// Utworzenie serwera web na porcie 80
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(82);

void setup() {
  Serial.begin(9600);

  // Inicjalizacja silników
  setupMotors();
  
  // Konfiguracja punktu dostępu WiFi
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress local_ip(LOCAL_IP_ADDRESS);
  IPAddress gateway(GATEWAY_ADDRESS);
  IPAddress subnet(SUBNET_MASK);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  
  delay(100);
  
  Serial.println("");
  Serial.print("Punkt dostępu WiFi uruchomiony z nazwą: ");
  Serial.println(AP_SSID);
  Serial.print("Adres IP: ");
  Serial.println(WiFi.softAPIP());
  
  // Inicjalizacja serwera webowego
  setupWebServer(server);

  // inicjalizowanie serwerów WebSocket
  setupWebSocketServer(webSocket);
  webSocket.begin();
  Serial.println("Serwer WebSocket uruchomiony na porcie 82");
}

void loop() {
  // Obsługa żądań klientów
  server.handleClient();
  webSocket.loop();
  delay(2);  // Małe opóźnienie dla stabilności
}
