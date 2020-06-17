# Agrumino lemon library

_Corsivo_

**Grassetto**

1. Uno
2. Due

* voce 1
  1. Sub 1
  2. Sub 2
* voce 2

Download Agrumino lib from [link1](https://drive.google.com/open?id=1on5ZwjzqYb_pVMqRCJAwRULhbYAZVMmM)

Follow instruction to install it at [link2](https://docs.google.com/document/d/1Dci24DAKnK0GN-E6B-5Q2Savg8v_qfiuaXvhIAeXK74/edit#)

Since our chip is WT8266 with a flash memory of 2MB it is necessary to modify the board settings:
substitute file at ~/.arduino15/packages/esp8266/hardware/esp8266/2.7.1/boards.txt with the one into ~/agruminolib/settingboard_WT8266/boards.txt

User need to restart ARDUINO IDE and select at Strumenti->Flash size: 2MB.

# Sketch to send data to dweet using FLASH

../agruminolib/examples/AgruminoDweetWithCaptiveWifiSample_FLASH/AgruminoDweetWithCaptiveWifiSample_FLASH.ino

Before execute the flash it is necessary to clean memory with ~/agruminolib/examples/memory_intializer_mod.ino 

//TO DO
- optimize code in order to make one commit instead of mutiple at each write of sensor value;
- verify if dweet accept a multiple Json (now we send a Json at each cycle of while - hours is the counter).


