#include <WiFi.h>
#include <AsyncUDP.h>
#include <Arduino.h>

const char *ssid = "ESP32";
const char *password = "01234567";
IPAddress addr(192, 168, 4, 1);

AsyncUDP udp;
const uint16_t port = 49152;

int data[4] = {0,0,0,0};


void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    digitalWrite(8,HIGH);
    delay(500);
    digitalWrite(8,LOW);
    delay(500);
  }

  if(udp.connect(addr, port))
  {
    Serial.println("UDP connected");
  }

  pinMode(0,INPUT); //x1
  pinMode(1,INPUT); //y1
  pinMode(3,INPUT); //x2
  pinMode(4,INPUT);//y2
  pinMode(8,OUTPUT);

}
void loop() {


  data[0]=map(analogRead(0),0,4095,-100,100);
  if(data[0]<4&&data[0]>0||data[0]>-4&&data[0]<0) data[0]=0;
  data[1]=map(analogRead(1),0,4095,-100,100);
  if(data[1]<4&&data[1]>0||data[1]>-4&&data[1]<0) data[1]=0;
  data[2]=map(analogRead(3),0,4095,-100,100);
  if(data[2]<4&&data[2]>0||data[2]>-4&&data[2]<0) data[2]=0;
  data[3]=map(analogRead(4),0,4095,-100,100);
  if(data[3]<4&&data[3]>0||data[3]>-4&&data[3]<0) data[3]=0;

  Serial.println(data[0]);
  Serial.print(" ");
  Serial.print(data[1]);
  Serial.print(" ");
  Serial.print(data[2]);
  Serial.print(" ");
  Serial.print(data[3]);

  digitalWrite(8,HIGH);

  udp.broadcastTo((uint8_t*)&data, sizeof(data), port);

  
  if (WiFi.status() != WL_CONNECTED) {
      setup();
  }

  delay(10);

}