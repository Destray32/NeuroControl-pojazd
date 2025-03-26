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

// wygenerowanie htmla z obsluga websocket do ruchu
String generateHtml()
{
    String html = "<html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-fa }";
    html += ".container { max-wmily: Arial, sans-serif; text-align: center; margin: 0; padding: 20px; }";
    html += "h1 { color: #333;idth: 600px; margin: 0 auto; }";
    html += ".controls { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; margin: 20px 0; }";
    html += ".btn { font-size: 18px; padding: 20px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; user-select: none; touch-action: manipulation; }";
    html += ".btn:active, .btn.active { background-color: #3e8e41; }";
    html += ".btn-stop { background-color: #f44336; }";
    html += ".btn-stop:active { background-color: #d32f2f; }";
    html += ".empty { visibility: hidden; }";
    html += ".status { margin-top: 20px; padding: 10px; background-color: #f1f1f1; border-radius: 5px; }";
    html += ".controls-title { margin-top: 20px; }";
    html += ".mode-controls { margin: 20px 0; }";
    html += "</style>";
    html += "</head><body>";
    
    html += "<div class='container'>";
    html += "<h1>Sterowanie NeuroVehicle</h1>";
    
    html += "<div class='mode-controls'>";
    html += "<button id='webMode' class='btn'>Sterowanie Web</button> ";
    html += "<button id='mlMode' class='btn'>Sterowanie ML</button>";
    html += "</div>";
    
    html += "<h2 class='controls-title'>Sterowanie WebSocket</h2>";
    html += "<div class='controls'>";
    html += "<div class='empty'></div>";
    html += "<button id='forward' class='btn'>GORA</button>";
    html += "<div class='empty'></div>";
    html += "<button id='left' class='btn'>LEWO</button>";
    html += "<button id='stop' class='btn btn-stop'>⬛</button>";
    html += "<button id='right' class='btn'>PRAWO</button>";
    html += "<div class='empty'></div>";
    html += "<button id='backward' class='btn'>DOL</button>";
    html += "<div class='empty'></div>";
    html += "</div>";
    
    html += "<div class='status' id='status'>Status: Rozłaczony</div>";
    html += "<div class='status' id='mode'>Tryb: Oczekiwanie...</div>";
    
    html += "<h3>Instrukcja</h3>";
    html += "<p>Mozesz tet uzyc klawiszy WSAD oraz spacji do sterowania.</p>";
    html += "</div>";
    
    // skrypty
    html += "<script>";
    html += "let ws = null;";
    html += "let currentMode = 'web';";
    html += "let isConnected = false;";
    
    html += "function connectWebSocket() {";
    html += "  const ip = window.location.hostname;";
    html += "  const port = 82;"; // port WS
    html += "  ws = new WebSocket(`ws://${ip}:${port}`);";
    
    html += "  ws.onopen = function() {";
    html += "    isConnected = true;";
    html += "    document.getElementById('status').innerText = 'Status: Polaczony';";
    html += "  };";
    
    html += "  ws.onclose = function() {";
    html += "    isConnected = false;";
    html += "    document.getElementById('status').innerText = 'Status: Rozlaczony';";
    html += "    setTimeout(connectWebSocket, 2000);"; // ponowne laczenie
    html += "  };";
    
    html += "  ws.onmessage = function(event) {";
    html += "    try {";
    html += "      const data = JSON.parse(event.data);";
    html += "      console.log('Otrzymano:', data);";
    
    html += "      if (data.status === 'connected' && data.mode) {";
    html += "        currentMode = data.mode;";
    html += "        updateModeDisplay();";
    html += "      }";
    
    html += "      if (data.status === 'mode_changed' && data.mode) {";
    html += "        currentMode = data.mode;";
    html += "        updateModeDisplay();";
    html += "      }";
    html += "    } catch (e) {";
    html += "      console.error('Błąd parsowania JSON:', e);";
    html += "    }";
    html += "  };";
    html += "}";
    
    // Aktualizacja wyświetlania trybu
    html += "function updateModeDisplay() {";
    html += "  document.getElementById('mode').innerText = 'Tryb: ' + (currentMode === 'web' ? 'Sterowanie Web' : 'Sterowanie ML');";
    html += "  document.getElementById('webMode').className = currentMode === 'web' ? 'btn active' : 'btn';";
    html += "  document.getElementById('mlMode').className = currentMode === 'ml' ? 'btn active' : 'btn';";
    html += "}";
    
    // Funkcja wysyłająca komendę
    html += "function sendCommand(command) {";
    html += "  if (!isConnected || currentMode !== 'web') return;";
    html += "  const msg = JSON.stringify({";
    html += "    source: 'web',";
    html += "    command: command";
    html += "  });";
    html += "  ws.send(msg);";
    html += "}";
    
    // Obsługa zdarzeń przycisków
    html += "document.getElementById('forward').addEventListener('mousedown', () => sendCommand('forward'));";
    html += "document.getElementById('backward').addEventListener('mousedown', () => sendCommand('backward'));";
    html += "document.getElementById('left').addEventListener('mousedown', () => sendCommand('left'));";
    html += "document.getElementById('right').addEventListener('mousedown', () => sendCommand('right'));";
    html += "document.getElementById('stop').addEventListener('mousedown', () => sendCommand('stop'));";
    
    // Obsługa zwolnienia przycisków (zatrzymanie)
    html += "document.getElementById('forward').addEventListener('mouseup', () => sendCommand('stop'));";
    html += "document.getElementById('backward').addEventListener('mouseup', () => sendCommand('stop'));";
    html += "document.getElementById('left').addEventListener('mouseup', () => sendCommand('stop'));";
    html += "document.getElementById('right').addEventListener('mouseup', () => sendCommand('stop'));";
    
    // Obsługa dotknięć mobilnych
    html += "document.getElementById('forward').addEventListener('touchstart', (e) => { e.preventDefault(); sendCommand('forward'); });";
    html += "document.getElementById('backward').addEventListener('touchstart', (e) => { e.preventDefault(); sendCommand('backward'); });";
    html += "document.getElementById('left').addEventListener('touchstart', (e) => { e.preventDefault(); sendCommand('left'); });";
    html += "document.getElementById('right').addEventListener('touchstart', (e) => { e.preventDefault(); sendCommand('right'); });";
    html += "document.getElementById('stop').addEventListener('touchstart', (e) => { e.preventDefault(); sendCommand('stop'); });";
    
    html += "document.getElementById('forward').addEventListener('touchend', (e) => { e.preventDefault(); sendCommand('stop'); });";
    html += "document.getElementById('backward').addEventListener('touchend', (e) => { e.preventDefault(); sendCommand('stop'); });";
    html += "document.getElementById('left').addEventListener('touchend', (e) => { e.preventDefault(); sendCommand('stop'); });";
    html += "document.getElementById('right').addEventListener('touchend', (e) => { e.preventDefault(); sendCommand('stop'); });";
    
    // Obsługa klawiszy
    html += "document.addEventListener('keydown', (e) => {";
    html += "  if (e.repeat) return;"; // Zapobieganie wielokrotnym zdarzeniom klawiatury
    html += "  switch(e.key.toLowerCase()) {";
    html += "    case 'w': sendCommand('forward'); break;";
    html += "    case 's': sendCommand('backward'); break;";
    html += "    case 'a': sendCommand('left'); break;";
    html += "    case 'd': sendCommand('right'); break;";
    html += "    case ' ': sendCommand('stop'); break;";
    html += "  }";
    html += "});";
    
    html += "document.addEventListener('keyup', (e) => {";
    html += "  switch(e.key.toLowerCase()) {";
    html += "    case 'w':";
    html += "    case 's':";
    html += "    case 'a':";
    html += "    case 'd':";
    html += "      sendCommand('stop');";
    html += "      break;";
    html += "  }";
    html += "});";
    
    // Przełączanie trybów
    html += "document.getElementById('webMode').addEventListener('click', () => {";
    html += "  if (isConnected) {";
    html += "    ws.send(JSON.stringify({mode: 'web'}));";
    html += "  }";
    html += "});";
    
    html += "document.getElementById('mlMode').addEventListener('click', () => {";
    html += "  if (isConnected) {";
    html += "    ws.send(JSON.stringify({mode: 'ml'}));";
    html += "  }";
    html += "});";
    
    // Inicjalizacja połączenia
    html += "connectWebSocket();";
    
    html += "</script>";
    html += "</body></html>";

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
