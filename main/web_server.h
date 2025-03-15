#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>

// Funkcje do obsługi serwera
void setupWebServer(WebServer& server);
void handleRoot();
void handleForward();
void handleBackward();
void handleLeft();
void handleRight();
void handleStop();
void handleNotFound();

// Funkcja do generowania strony HTML
String generateHtml();

#endif // WEB_SERVER_H