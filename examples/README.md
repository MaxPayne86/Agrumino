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
Install AsyncElegantOTA library

* HTTP Server  checks for updates and download a binary file from HTTP web server. It is possible to download updates from every IP or domain address.

### Update process

1. The new sketch will be stored in the space between the old sketch and the spiff.
2. on the next reboot, the “eboot” bootloader checks for commands.
3. the new sketch is now copied “over” the old one.
4. the new sketch is started.

Further informations [here](https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html)

### Accessing wifi network

For accessing the wifi network we use the WiFiManager library which manages the connection and contains all html, css, icons files required.
In order to connect the Agrumino board to the wifi network, we can follow these steps:
1. at start up, it sets the board up in Station mode and tries to connect to a previously saved Access Point
2. if this is unsuccessful (or no previous network saved) it moves the Agrumino board into Access Point mode and spins up a DNS and WebServer (default ip 192.168.4.1)
3. using any wifi enabled device with a browser (computer, phone, tablet) connect to the newly created Access Point (which has a SSID such as Agrumino-XXXXXX, where XXXXX is a number)
4. because of the Captive Portal and the DNS server you will either get a 'Join to network' type of popup or get any domain you try to access redirected to the configuration portal
5. choose one of the access points scanned, enter password, click save
6. Agrumino will try to connect. If successful, it stops control back your app. If not, reconnect to AP and reconfigure.

Further informations at: https://github.com/tzapu/WiFiManager

###Thing Speak Sketch with JSON POST every 4 sample

In order to get the data and time of each collect of sensor data, it is necessary to download a new library for Arduino: 
- NTPClient library forked by Taranais[link](https://github.com/taranais/NTPClient )


















