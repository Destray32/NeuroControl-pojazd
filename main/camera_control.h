#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

#define CAMERA_MODEL_WROVER_KIT 

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "camera_related/camera_pins.h"
#include "esp_camera.h"

// funkcje do obsługi kamery
bool setupCamera();
void streamMJPEG(WiFiClient client);
void handleCapture(WebServer& server);
void handleResolution(WebServer& server);
void handleStream(WebServer& server);
void handleNotFound(WebServer& server);
void startCameraServer(WebServer& server);

#endif // CAMERA_CONTROL_H