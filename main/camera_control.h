#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "esp_camera.h"

// piny kamery
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     13
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       14
#define VSYNC_GPIO_NUM    27
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// funkcje do obsługi kamery
bool setupCamera();
void startCameraServer(WebServer& server);

#endif // CAMERA_CONTROL_H