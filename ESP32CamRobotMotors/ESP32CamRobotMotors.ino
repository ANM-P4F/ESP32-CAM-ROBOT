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

#include <AFMotor.h>
#include <ArduinoSTL.h> //install arduinoSTL
#include <Servo.h>

#define DEBUG
#define PIN_PICKL 10
#define PIN_PICKR 9

AF_DCMotor motor[2] = {AF_DCMotor(4,MOTOR34_8KHZ),AF_DCMotor(3,MOTOR34_8KHZ)};
Servo servoPickL;
Servo servoPickR;

const float FORCE_Y = 1.5f;
const float FORCE_X = 0.8f;

std::vector<String> splitString(String data, String delimiter);
void setSpeed(int ax, int ay);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.setTimeout(50);
  motor[0].run(RELEASE);
  motor[1].run(RELEASE);
  setMotor(0, 0) ;
  setMotor(1, 0) ;
  servoPickL.attach(PIN_PICKL);
  servoPickR.attach(PIN_PICKR);
  servoPickL.write(0);
  servoPickR.write(30);
}

void loop() {
  if (Serial.available()) {
    String data = Serial.readString();
    if(data.indexOf("MOVE")!=-1){
      String posVals = getValue(data,':',1);
      std::vector<String> vposVals = splitString(posVals, ",");
      Serial.println(data);
      if(vposVals.size() != 2){
        return;
      }
      int x = vposVals[0].toInt();
      int y = vposVals[1].toInt();
      setSpeed(x,y);
    }else if(data.indexOf("TEST")!=-1){
      String motorVals = getValue(data,':',1);
      std::vector<String> vMotorVals = splitString(motorVals, ",");
      for (int i = 0; i < vMotorVals.size(); i++){
        // Serial.println(vMotorVals[i]);
        if (i>2) break;
        setMotor(i, vMotorVals[i].toInt()) ;
      }
    }else if(data.indexOf("PICK")!=-1){
      String pickValue = getValue(data,':',1);
      if(pickValue.indexOf("ON")!=-1){
      servoPickL.write(30);
      servoPickR.write(0);
      }else if(pickValue.indexOf("OFF")!=-1){
        servoPickL.write(0);
        servoPickR.write(30);
      }
    }
  }
}

void setSpeed(int Vx, int Vy){
  float f1 = FORCE_Y*Vy-FORCE_X*Vx;
  float f2 = FORCE_Y*Vy+FORCE_X*Vx;
  int pwm0 = int(f1)>180 ? 180 : int(f1);
  int pwm1 = int(f2)>180 ? 180 : int(f2);
  setMotor(0, pwm0);
  setMotor(1, pwm1);
  String motors = String("Motors: ") + String(Vx) + String(",") + String(Vy) + String(",") + String(pwm0) + String(",")+ String(pwm1);
  Serial.println(motors);
}

void setMotor(int indx, int val)
{
    motor[indx].setSpeed(abs(val));
    if(val>0){
      motor[indx].run(FORWARD);
    }else{
      motor[indx].run(BACKWARD);
    }
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

std::vector<String> splitString(String data, String delimiter){
    std::vector<String> ret;
    // initialize first part (string, delimiter)
    char* ptr = strtok(data.c_str(), delimiter.c_str());

    while(ptr != NULL) {
        ret.push_back(String(ptr));
        // create next part
        ptr = strtok(NULL, delimiter.c_str());
    }
    return ret;
}

