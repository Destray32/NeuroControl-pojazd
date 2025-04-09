#include "camera_control.h"

int streamingClients = 0;
bool isStreaming = false;

const char CAMERA_INDEX_HTML[] = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <title>NeuroVehicle Camera</title>
      <style>
        body { font-family: Arial, Helvetica, sans-serif; text-align: center; margin: 0; padding: 0; }
        .navbar { background-color: #333; overflow: hidden; position: fixed; top: 0; width: 100%; }
        .navbar a { float: left; display: block; color: #f2f2f2; text-align: center; padding: 14px 16px; text-decoration: none; }
        .navbar a:hover { background-color: #ddd; color: black; }
        .content { padding: 60px 20px 20px; }
        img { max-width: 100%; height: auto; border: 1px solid #ddd; border-radius: 4px; padding: 5px; }
        .stream-container { margin-top: 20px; }
        .resolution-selector { margin-bottom: 20px; }
      </style>
    </head>
    <body>
      <div class="navbar">
        <a href="/">Home</a>
        <a href="/camera" class="active">Camera</a>
        <a href="/api/docs">API Docs</a>
        <a href="/control">Control</a>
      </div>
      <div class="content">
        <h1>NeuroVehicle Camera Stream</h1>
        <div class="resolution-selector">
          <label for="resolution">Resolution:</label>
          <select id="resolution" onchange="changeResolution(this.value)">
            <option value="UXGA">UXGA (1600x1200)</option>
            <option value="SXGA">SXGA (1280x1024)</option>
            <option value="XGA">XGA (1024x768)</option>
            <option value="SVGA" selected>SVGA (800x600)</option>
            <option value="VGA">VGA (640x480)</option>
            <option value="CIF">CIF (400x296)</option>
            <option value="QVGA">QVGA (320x240)</option>
          </select>
        </div>
        <div class="stream-container">
          <img id="stream" src="/stream" alt="Camera Stream">
        </div>
      </div>
      <script>
        function changeResolution(resolution) {
          fetch(`/camera/resolution?res=${resolution}`, {
            method: 'GET'
          })
          .then(response => {
            if (response.ok) {
              const streamImg = document.getElementById('stream');
              // Add timestamp to bust cache
              streamImg.src = `/stream?t=${new Date().getTime()}`;
            } else {
              alert('Failed to change resolution');
            }
          })
          .catch(error => {
            console.error('Error:', error);
          });
        }
      </script>
    </body>
    </html>
    )rawliteral";

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
    config.pixel_format = PIXFORMAT_JPEG;

    if (psramFound())
    {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 10; // 0-63, niżej to lepsza jakość
        config.fb_count = 2;
    }
    else
    {
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    // inicjalizacja kamery
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, etc)
    s->set_whitebal(s, 1);       // 0 = disable, 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable, 1 = enable
    s->set_wb_mode(s, 0);        // 0 to 4 - auto, sunny, cloudy, office, home
    s->set_exposure_ctrl(s, 1);  // 0 = disable, 1 = enable
    s->set_aec2(s, 0);           // 0 = disable, 1 = enable
    s->set_ae_level(s, 0);       // -2 to 2
    s->set_aec_value(s, 300);    // 0 to 1200
    s->set_gain_ctrl(s, 1);      // 0 = disable, 1 = enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);            // 0 = disable, 1 = enable
    s->set_wpc(s, 1);            // 0 = disable, 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable, 1 = enable
    s->set_lenc(s, 1);           // 0 = disable, 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable, 1 = enable
    s->set_vflip(s, 0);          // 0 = disable, 1 = enable
    s->set_dcw(s, 1);            // 0 = disable, 1 = enable
    s->set_colorbar(s, 0);       // 0 = disable, 1 = enable

    return true;
}

// funkcja odpowiedzialna za przechwytywanie i strumowanie klatki
void captureAndStreamFrame(WiFiClient client)
{
    camera_fb_t* fb = esp_camera_fb_get(); // przechwycenie klatki
    if (!fb)
    {
        Serial.println("Camera capture failed");
        return;
    }

    client.write((const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
}

// funkcja odpowiedzialna za strumowanie MJPEG
// do klienta, który się połączył
void streamMJPEG(WiFiClient client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Access-Control-Allow-Origin: *");
  client.println("");

  streamingClients++;
  isStreaming = true;

  while(client.connected())
  {
    camera_fb_t* fb = esp_camera_fb_get(); // przechwycenie klatki
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
    esp_camera_fb_return(fb); // zwolnienie bufora klatki
    delay(33); // opóźnienie dla stabilności strumienia
  }

  streamingClients--;
  if (streamingClients == 0)
  {
    isStreaming = false;
    Serial.println("No clients connected, stopping stream.");
  }
}

// funkcja do wysłania pojedynczej klatki
void handleCapture(WebServer& server) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
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
void handleResolution(WebServer& server) {
  String res = server.arg("res");
  framesize_t frameSize;
  
  if (res == "UXGA") frameSize = FRAMESIZE_UXGA;
  else if (res == "SXGA") frameSize = FRAMESIZE_SXGA;
  else if (res == "XGA") frameSize = FRAMESIZE_XGA;
  else if (res == "SVGA") frameSize = FRAMESIZE_SVGA;
  else if (res == "VGA") frameSize = FRAMESIZE_VGA;
  else if (res == "CIF") frameSize = FRAMESIZE_CIF;
  else if (res == "QVGA") frameSize = FRAMESIZE_QVGA;
  else {
    server.send(400, "text/plain", "Invalid resolution");
    return;
  }
  
  sensor_t * s = esp_camera_sensor_get();
  if (s->set_framesize(s, frameSize) != 0) {
    server.send(500, "text/plain", "Failed to set resolution");
    return;
  }
  
  server.send(200, "text/plain", "Resolution set to " + res);
}

// odpalenie streamowania MJPEG na życzenie klienta
void handleStream(WebServer& server) {
  WiFiClient client = server.client();
  
  if (!client.connected()) {
    return;
  }
  
  streamMJPEG(client);
}

void handleCameraPage(WebServer& server) {
  server.send(200, "text/html", CAMERA_INDEX_HTML);
}

void startCameraServer(WebServer& server) {
  server.on("/camera", HTTP_GET, [&]() { handleCameraPage(server); });
  server.on("/capture", HTTP_GET, [&]() { handleCapture(server); });
  server.on("/stream", HTTP_GET, [&]() { handleStream(server); });
  server.on("/camera/resolution", HTTP_GET, [&]() { handleResolution(server); });
}
