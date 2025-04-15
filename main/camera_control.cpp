#include "camera_control.h"

int streamingClients = 0;
bool isStreaming = false;

bool setupCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 20;
  config.fb_count = 1;

// if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 20;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_QVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return false;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

  Serial.printf("Camera initialized successfully\n");
  return true;
}

// funkcja odpowiedzialna za przechwytywanie i strumowanie klatki
void captureAndStreamFrame(WiFiClient client)
{
  camera_fb_t *fb = esp_camera_fb_get(); // przechwycenie klatki
  if (!fb)
  {
    Serial.println("Camera capture failed");
    return;
  }

  client.write((const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

// funkcja odpowiedzialna za strumowanie MJPEG
// do klienta, który się połączył
void streamMJPEG(WiFiClient client)
{
  sensor_t *s = esp_camera_sensor_get();
  framesize_t original_framesize = s->status.framesize;
  s->set_framesize(s, FRAMESIZE_QVGA); // Force QVGA for streaming
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Access-Control-Allow-Origin: *");
  client.println("");

  streamingClients++;
  isStreaming = true;

  while (client.connected())
  {
    camera_fb_t *fb = esp_camera_fb_get(); // przechwycenie klatki
    if (!fb)
    {
      Serial.println("Camera capture failed");
      break;
    }
    client.println();
    client.println("--frame");
    client.print("Content-Type: image/jpeg\r\n");
    client.print("Content-Length: ");
    client.println(fb->len);
    client.println();

    client.write(fb->buf, fb->len); // wysłanie klatki do klienta
    esp_camera_fb_return(fb);       // zwolnienie bufora klatki
    delay(100);                      // opóźnienie dla stabilności strumienia
  }

  streamingClients--;
  if (streamingClients == 0)
  {
    isStreaming = false;
    Serial.println("No clients connected, stopping stream.");
  }
}

// funkcja do wysłania pojedynczej klatki
void handleCapture(WebServer &server)
{
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  server.setContentLength(fb->len);
  server.send(200, "image/jpeg", "");
  server.sendContent((const char *)fb->buf, fb->len);

  esp_camera_fb_return(fb);
}

// funckja do zmiany rozdzielczości kamery
// na podstawie argumentu `res`
void handleResolution(WebServer &server)
{
  String res = server.arg("res");
  framesize_t frameSize;

  if (res == "UXGA")
    frameSize = FRAMESIZE_UXGA;
  else if (res == "SXGA")
    frameSize = FRAMESIZE_SXGA;
  else if (res == "XGA")
    frameSize = FRAMESIZE_XGA;
  else if (res == "SVGA")
    frameSize = FRAMESIZE_SVGA;
  else if (res == "VGA")
    frameSize = FRAMESIZE_VGA;
  else if (res == "CIF")
    frameSize = FRAMESIZE_CIF;
  else if (res == "QVGA")
    frameSize = FRAMESIZE_QVGA;
  else
  {
    server.send(400, "text/plain", "Invalid resolution");
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s->set_framesize(s, frameSize) != 0)
  {
    server.send(500, "text/plain", "Failed to set resolution");
    return;
  }

  server.send(200, "text/plain", "Resolution set to " + res);
}

// odpalenie streamowania MJPEG na życzenie klienta
void handleStream(WebServer &server)
{
  WiFiClient client = server.client();

  if (!client.connected())
  {
    return;
  }

  streamMJPEG(client);
}

void handleNotFound(WebServer &server)
{
  server.send(404, "text/plain", "Not Found");
}

void startCameraServer(WebServer &server)
{
  server.on("/capture", HTTP_GET, [&]()
            { handleCapture(server); });
  server.on("/stream", HTTP_GET, [&]()
            { handleStream(server); });
  server.on("/camera/resolution", HTTP_GET, [&]()
            { handleResolution(server); });
  server.onNotFound([&]()
                    { handleNotFound(server); });
}
