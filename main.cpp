#include "esp_camera.h"
#include<WiFi.h>
#include<AsyncUDP.h>
#include<WebServer.h>
#include "img_converters.h"
#include <ESP32Servo.h>

#define CAMERA_MODEL_AI_THINKER

#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27

#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM    5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22


  const char* ssid = "ESP32";
  const char* password = "01234567";
  AsyncUDP udp;
  const uint16_t port = 49152;
  char rx_buffer[128];
  int control[10];
  WebServer server(80);
  int* pdata;
  


// HTML-страница
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Камера</title>
  <style>
    body { text-align: center; font-family: Arial; background-color: #222; color: white; }
    img { border: 4px solid #444; border-radius: 10px; margin-top: 20px; }
  </style>
</head>
<body>
  <h1>ESP32 Видеопоток</h1>
  <img src="/stream" width="320" height="240">
</body>
</html>
)rawliteral";


void parsePacket(AsyncUDPPacket packet)
{
    pdata = (int*)packet.data();
    const size_t len = packet.length();

  if (pdata != NULL && len > 0) 
  {
    Serial.println("recieved");
    Serial.println(*pdata);
    Serial.print("length : ");
    Serial.println(len);

        const size_t len = packet.length() / sizeof(&pdata);

    if (pdata != NULL && len > 0) 
    {
        for (size_t i = 0; i < len; i++) 
        {
            Serial.print(pdata[i]);   
            Serial.print(", ");
        }
        Serial.println("");
    }
  }
}



void startCamera() 
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size   = FRAMESIZE_QQVGA; 
  config.jpeg_quality = 12;             
  config.fb_count     = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Ошибка камеры: 0x%x", err);
    return;
  }
  Serial.println("камера успешно инициализирована");
}

// Обработчик потока
void handleStream() {
  WiFiClient client = server.client();

  String response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
    "\r\n";
  client.print(response);

  const unsigned long frameInterval = 50; // 30 fps → 1000 / 30 ≈ 33ms
  unsigned long lastFrameTime = 0;

  while (client.connected()) {
    unsigned long now = millis();
    if (now - lastFrameTime >= frameInterval) {
      lastFrameTime = now;

      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Не удалось получить кадр");
        continue;
      }

      uint8_t* jpeg_buffer = NULL;
      size_t jpeg_len = 0;

      if (frame2jpg(fb, 12, &jpeg_buffer, &jpeg_len)) {
        client.print("--frame\r\n");
        client.print("Content-Type: image/jpeg\r\n");
        client.print("Content-Length: " + String(jpeg_len) + "\r\n\r\n");
        client.write(jpeg_buffer, jpeg_len);
        client.print("\r\n");

        free(jpeg_buffer);
      } else {
        Serial.println("Ошибка преобразования в JPEG");
      }

      esp_camera_fb_return(fb);
    }

  }
}



void setup() 
{

  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());

  startCamera();
  server.on("/", HTTP_GET, []() {
  server.send_P(200, "text/html", index_html);
   });
  server.on("/stream", HTTP_GET, handleStream);
  Serial.println("сервер запущен обработчик");
  server.begin();
  Serial.println("сервер запущен");

  if(udp.listen(port)) 
  {
    udp.onPacket(parsePacket);
  }


  ledcSetup(1, 50, 16);
  ledcAttachPin(14, 1);
  ledcSetup(2, 50, 16);
  ledcAttachPin(13, 2);


}

long long int lasttime = 0;
long long int now = 0;

void loop() 
{
  now = millis();
  server.handleClient();
  int duty_motor=0;
  int duty_servo=0;
  int mic_motor=0;
  int mic_servo=0;

  if (now - lasttime >= 10)
  {
  lasttime = now;
  if(pdata != NULL)
    {
      mic_motor = map (pdata[1], 0, 100, 1000, 2000);
      mic_servo = map (pdata[2], -100, 100, 1000, 2000);
      duty_motor = (mic_motor * 65535) / 20000;
      duty_servo = (mic_servo * 65535) / 20000;
    }
    
    
    ledcWrite(1, duty_motor);
    ledcWrite(2, duty_servo);

    Serial.println("Power ");
    Serial.print(duty_motor);
  }



}
