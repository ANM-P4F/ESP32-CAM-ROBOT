/*
BSD 2-Clause License

Copyright (c) 2020, ANM-P4F
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <WebSocketsServer.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <FS.h>
#include <ESPAsyncWebServer.h>
#include "camera_wrap.h"

#define DEBUG
// #define SAVE_IMG

const char* ssid = "your ssid";    // <<< change this as yours
const char* password = "your ssid password"; // <<< change this as yours
//holds the current upload
int cameraInitState = -1;
uint8_t* jpgBuff = new uint8_t[68123];
size_t   jpgLength = 0;
uint8_t camNo=0;
bool clientConnected[3] = {false,false,false};

WebSocketsServer webSocket = WebSocketsServer(86);
AsyncWebServer httpServer(80);
String html_home;

const int PIN_SERVO   = 12;
const int PIN_LED     = 2;
const int SERVO_RESOLUTION    = 16;
unsigned long previousMillisServo = 0;
const unsigned long intervalServo = 100;
bool reqUp = false;
bool reqDown = false;
int posServo = 90;
uint8_t angleMax = 180;

void setup(void) {

  Serial.begin(115200);
  Serial.print("\n");
  Serial.setDebugOutput(true);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file){
      Serial.print("FILE: ");
      Serial.println(file.name());
      file = root.openNextFile();
    }
  }

  cameraInitState = initCamera();

  Serial.printf("camera init state %d\n", cameraInitState);

  if(cameraInitState != 0){
    return;
  }

  //WIFI INIT
  Serial.printf("Connecting to %s\n", ssid);
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  String ipAddress = WiFi.localIP().toString();;
  Serial.println(ipAddress);
  // handle index
  // Route for root / web page
  httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });
  //handle icons load
  httpServer.on("/icons/led_on.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/icons/led_on.png", "image/png");
  });
  httpServer.on("/icons/led_off.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/icons/led_off.png", "image/png");
  });
  httpServer.on("/icons/move_off.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/icons/move_off.png", "image/png");
  });
  httpServer.on("/icons/pick_on.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/icons/pick_on.png", "image/png");
  });
  httpServer.on("/icons/pick_off.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/icons/pick_off.png", "image/png");
  });
  httpServer.on("/icons/up_on.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/icons/up_on.png", "image/png");
  });
  httpServer.on("/icons/up_off.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/icons/up_off.png", "image/png");
  });
  httpServer.on("/icons/down_on.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/icons/down_on.png", "image/png");
  });
  httpServer.on("/icons/down_off.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/icons/down_off.png", "image/png");
  });
  //handle command 
  httpServer.on("/MOVE", HTTP_GET, [](AsyncWebServerRequest *request){
    commadProcessing(request);
    request->send(200, "text/plain");
  });
  httpServer.on("/LED", HTTP_GET, [](AsyncWebServerRequest *request){
    commadProcessing(request);
    request->send(200, "text/plain");
  });
  httpServer.on("/PICK", HTTP_GET, [](AsyncWebServerRequest *request){
    commadProcessing(request);
    request->send(200, "text/plain");
  });
  httpServer.on("/UP", HTTP_GET, [](AsyncWebServerRequest *request){
    commadProcessing(request);
    request->send(200, "text/plain");
  });
  httpServer.on("/DOWN", HTTP_GET, [](AsyncWebServerRequest *request){
    commadProcessing(request);
    request->send(200, "text/plain");
  });

  httpServer.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // 1. 50hz ==> period = 20ms (sg90 servo require 20ms pulse, duty cycle is 1->2ms: -90=>90degree)
  // 2. resolution = 16, maximum value is 2^16-1=65535
  // From 1 and 2 => -90=>90 degree or 0=>180degree ~ 3276=>6553
  ledcSetup(4, 50, SERVO_RESOLUTION);//channel, freq, resolution
  ledcAttachPin(PIN_SERVO, 4);// pin, channel

  pinMode(PIN_LED, OUTPUT);
}

void servoWrite(uint8_t channel, uint8_t value) {
  // regarding the datasheet of sg90 servo, pwm period is 20 ms and duty is 1->2ms
  uint32_t maxDuty = (pow(2,SERVO_RESOLUTION)-1)/10; 
  uint32_t minDuty = (pow(2,SERVO_RESOLUTION)-1)/20; 
  uint32_t duty = (maxDuty-minDuty)*angle/180 + minDuty;
  ledcWrite(channel, duty);
}

void controlServo(){
  if(reqUp){
    if(posServo<180){
      posServo -= 1;
    }
  }
  if(reqDown){
    if(posServo>0){
      posServo += 1;
    }
  }
  
  servoWrite(4,posServo);
}

void commadProcessing(AsyncWebServerRequest *request){
  String value = request->getParam(0)->value();
  String url   = request->url();
  if(url.indexOf("UP")!=-1){
    if(value.indexOf("ON")!=-1){
      reqUp = true;
    }else if(value.indexOf("OFF")!=-1){
      reqUp = false;
    }
  }else if(url.indexOf("DOWN")!=-1){
    if(value.indexOf("ON")!=-1){
      reqDown = true;
    }else if(value.indexOf("OFF")!=-1){
      reqDown = false;
    }
  }else if(url.indexOf("LED")!=-1){
    if(value.indexOf("ON")!=-1){
      digitalWrite(PIN_LED, HIGH);
    }else if(value.indexOf("OFF")!=-1){
      digitalWrite(PIN_LED, LOW);
    }
  }else{
    String command = url+":"+value+"\n";
    Serial.print(command);
  }

}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
      case WStype_DISCONNECTED:
          Serial.printf("[%u] Disconnected!\n", num);
          if(num<3){
            camNo = num;
            clientConnected[num] = false;
            String command = "/MOVE:0,0";
            Serial.print(command);
          }
          break;
      case WStype_CONNECTED:
          if(num<3){
            clientConnected[num] = true;
          }
          break;
      case WStype_TEXT:
      case WStype_BIN:
      case WStype_ERROR:      
      case WStype_FRAGMENT_TEXT_START:
      case WStype_FRAGMENT_BIN_START:
      case WStype_FRAGMENT:
      case WStype_FRAGMENT_FIN:
          break;
  }
}

void loop(void) {
  webSocket.loop();
  if(clientConnected[camNo] == true){
    grabImage(jpgLength, jpgBuff);
    webSocket.sendBIN(camNo, jpgBuff, jpgLength);
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisServo >= intervalServo) {
    previousMillisServo = currentMillis;
    controlServo();
  }
}

