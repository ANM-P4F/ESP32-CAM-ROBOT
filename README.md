# ESP32-CAM Simple Surveillance RC Robot #

## Demonstration
[![ESP32-CAM: Simple Surveillance RC Car](http://img.youtube.com/vi/xvkwcLCyFT0/0.jpg)](https://www.youtube.com/watch?v=xvkwcLCyFT0 "ESP32-CAM: Simple Surveillance RC Car")

## Dependencies

1. For ESP32-CAM module:
Download https://github.com/Links2004/arduinoWebSockets as zip file and add this library to your Arduino Libraries by Sketch/Include Library/Add Zip...<br/>
Download https://github.com/me-no-dev/ESPAsyncWebServer as zip file and add this library to your Arduino Libraries by Sketch/Include Library/Add Zip...<br/>
Download https://github.com/me-no-dev/AsyncTCP as zip file and add this library to your Arduino Libraries by Sketch/Include Library/Add Zip...<br/>

2. For Arduino module:<br/>
Install arduinoSTL by [Manage Libraries...]<br/>

## SPIFFS upload tool:
Download and extract in [your arduino ide folder]/tools.
https://github.com/me-no-dev/arduino-esp32fs-plugin/releases/

## How to upload the firmware:

1. ESP32-CAM module firmware(ESP32CamRobot): contains 2 parts <br/>
 - data: contains the GUI (html, icon), use SPIFFS upload tool to upload to the module <br/>
 - ESP32CamRobot sketch: normally upload by arduino IDE <br/>
2. Arduino Uno firmware (ESP32CamRobotMotors): normally upload by arduino IDE <br/>

## How to run:
Use Arduino IDE serial monitor tool to capture the serial data from ESP32-CAM module on starting up to get the camera IP address. <br/>
Use your webbrowser to connect to the that IP.

## LICENSE
The part contains my code is released under BSD 2-Clause License. Regarding other libraries used in this project, please follow the respective Licenses.
