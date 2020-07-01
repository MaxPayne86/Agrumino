## OTA

OTA (Over the Air) update allows to update firmware to an Agrumino board using a Wi-Fi connection instead of a serial port.

OTA may be done using:

1. Arduino IDE
2. Web Browser
3. HTTP Server

* Arduino IDE  option is intended primarily for the software development phase.
Here is an example of the command line the Arduino IDE runs to upload the sketch on the Agrumino board:

```
~/.arduino15/packages/esp8266/tools/python3/3.7.2-post1/python3 ~/.arduino15/packages/esp8266/hardware/esp8266/2.7.1/tools/espota.py -i 192.168.1.7 -p 8266 --auth= -f /tmp/arduino_build_261508/sketch_to_upload.ino.bin
```

where 192.168.1.7 is the IP address assigned from wifi network to the board and 8266 is the port

* Web Browser  updates are done with a web browser
AgruminoDweet_OTA_web.ino is the example sketch.
This sketch reads every 1h all values from the Arduino board and update them to the Dweet.io service every 4h. 
It integrates FLASH management to collect all data before transmit them. 
Moreover integrates the update of the firmware via OTA using a web page.
To enter UPDATE Modality you need to press RESET Button (S2) and immeditely press repetitively USER Button (S1). When in UPDATE Mod led will blink faster.
Then copy one of the two links after sentence "My Update Page is:" on Serial Monitor and paste it on your web browser.
It is necessary to select binary file from local disk compiled by Arduino IDE (ex. /tmp/arduino_build_815021/AgruminoDweet_OTA_web.ino).
Then select update button and after reboot new sketch will be executed on Agrumino board.

* HTTP Server  checks for updates and download a binary file from HTTP web server. It is possible to download updates from every IP or domain address.

### Update process

1. The new sketch will be stored in the space between the old sketch and the spiff.
2. on the next reboot, the “eboot” bootloader checks for commands.
3. the new sketch is now copied “over” the old one.
4. the new sketch is started.

Further informations [here](https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html)
