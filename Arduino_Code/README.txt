This folder contains:
1. The Arduino Nano code. 
2. The CH340g driver from https://github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver

In order to use the code, you must install the following libraries:
  1. Adafruit SSD1306     -- by Adafruit Version 1.0.1
  2. Adafruit GFX Library -- by Adafruit Version 1.0.2
  3. TimerOne             -- by Jesse Tane et al. Version 1.1.0

This can be done from the Arduino IDE: go to Tools, select
  1. Board = Arduino nano
  2. Port = the USB port that the Arduino is plugged into. If you don’t see it, you may need to install the CH340g driver

If you are using MAC OS, you need to also install the CH340g Driver. 

IF you are using Linux, you need to give permission to use your USB ports:
	In terminal: sudo chmod 666 /dev/ttyUSB0