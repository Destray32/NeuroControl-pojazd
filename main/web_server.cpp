#include "web_server.h"
#include "motor_control.h"
#include "api_handler.h"
#include <WebServer.h>

// Deklaracja referencji do obiektu WebServer
WebServer *server_ptr = nullptr;

// Funkcja inicjalizująca serwer web
void setupWebServer(WebServer &server)
{
    // Zapisz referencję do obiektu serwera
    server_ptr = &server;

    // Definicja endpointów serwera
    server.on("/", handleRoot);
    server.on("/forward", handleForward);
    server.on("/backward", handleBackward);
    server.on("/left", handleLeft);
    server.on("/right", handleRight);
    server.on("/stop", handleStop);
    
    // Konfiguracja endpointów API
    setupAPIEndpoints(server);
    
    server.onNotFound(handleNotFound); // Obsługa nieznalezionych endpointów

    // Uruchomienie serwera
    server.begin();
    Serial.println("Serwer HTTP uruchomiony");
}

// Generowanie strony HTML
String generateHtml()
{
    String html = "<html><body style='font-family: Arial; text-align: center;'>";
    html += "<h1>Sterowanie NeuroVehicle</h1>";
    html += "<div style='margin: 20px;'>";
    html += "<button style='font-size: 24px; padding: 20px; margin: 10px;' onclick=\"location.href='/forward'\">Do przodu</button><br>";
    html += "<button style='font-size: 24px; padding: 20px; margin: 10px;' onclick=\"location.href='/left'\">W lewo</button>";
    html += "<button style='font-size: 24px; padding: 20px; margin: 10px;' onclick=\"location.href='/stop'\">STOP</button>";
    html += "<button style='font-size: 24px; padding: 20px; margin: 10px;' onclick=\"location.href='/right'\">W prawo</button><br>";
    html += "<button style='font-size: 24px; padding: 20px; margin: 10px;' onclick=\"location.href='/backward'\">Do tyłu</button>";
    html += "</div></body></html>";

    return html;
}

// Handler dla strony głównej
void handleRoot()
{
    if (server_ptr)
    {
        server_ptr->send(200, "text/html", generateHtml());
    }
}

// Handler dla endpointu jazdy do przodu
void handleForward()
{
    moveForward();
    if (server_ptr)
    {
        server_ptr->send(200, "text/plain", "Jazda do przodu");
    }
}

// Handler dla endpointu jazdy do tyłu
void handleBackward()
{
    moveBackward();
    if (server_ptr)
    {
        server_ptr->send(200, "text/plain", "Jazda do tyłu");
    }
}

// Handler dla endpointu skrętu w lewo
void handleLeft()
{
    turnLeft();
    if (server_ptr)
    {
        server_ptr->send(200, "text/plain", "Skręt w lewo");
    }
}

// Handler dla endpointu skrętu w prawo
void handleRight()
{
    turnRight();
    if (server_ptr)
    {
        server_ptr->send(200, "text/plain", "Skręt w prawo");
    }
}

// Handler dla endpointu zatrzymania
void handleStop()
{
    stopMotors();
    if (server_ptr)
    {
        server_ptr->send(200, "text/plain", "Zatrzymano");
    }
}

// Handler dla nieznalezionych endpointów
void handleNotFound()
{
    if (server_ptr)
    {
        server_ptr->send(404, "text/plain", "Nie znaleziono");
    }
}
