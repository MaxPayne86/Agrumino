## Accessing wifi network

For accessing the wifi network we use the WiFiManager library which manages the connection and contains all html, css, icons files required.
In order to connect the Agrumino board to the wifi network, we can follow these steps:
1. at start up, it sets the board up in Station mode and tries to connect to a previously saved Access Point
2. if this is unsuccessful (or no previous network saved) it moves the Agrumino board into Access Point mode and spins up a DNS and WebServer (default ip 192.168.4.1)
3. using any wifi enabled device with a browser (computer, phone, tablet) connect to the newly created Access Point (which has a SSID such as Agrumino-XXXXXX, where XXXXX is a number)
4. because of the Captive Portal and the DNS server you will either get a 'Join to network' type of popup or get any domain you try to access redirected to the configuration portal
5. choose one of the access points scanned, enter password, click save
6. Agrumino will try to connect. If successful, it stops control back your app. If not, reconnect to AP and reconfigure.

Further informations at: https://github.com/tzapu/WiFiManager


















