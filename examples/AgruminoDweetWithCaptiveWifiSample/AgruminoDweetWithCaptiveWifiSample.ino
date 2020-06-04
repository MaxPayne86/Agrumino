/*
  AgruminoCaptiveWiSample.ino - Sample project for Agrumino board using the Agrumino library.
  Created by giuseppe.broccia@lifely.cc on October 2017.

  @see Agrumino.h for the documentation of the lib
*/
#include "Agrumino.h"           // Our super cool lib ;)
#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <DNSServer.h>          // Installed from ESP8266 board
#include <ESP8266WebServer.h>   // Installed from ESP8266 board
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson

// Time to sleep in second between the readings/data sending
#define SLEEP_TIME_SEC 300 // 5 min


// Web Server data, in our sample we use Dweet.io.
const char* WEB_SERVER_HOST = "dweet.io";
const String WEB_SERVER_API_SEND_DATA = "/dweet/quietly/for/"; // The Dweet name is created in the loop() method.

// Our super cool lib
Agrumino agrumino;

// Used for sending Json POST requests
StaticJsonBuffer<200> jsonBuffer;
// Used to create TCP connections and make Http calls
WiFiClient client;

void setup() {

  Serial.begin(115200);

  // Setup our super cool lib
  agrumino.setup();

  // Turn on the board to allow the usage of the Led
  agrumino.turnBoardOn();

  // WiFiManager Logic taken from
  // https://github.com/kentaylor/WiFiManager/blob/master/examples/ConfigOnSwitch/ConfigOnSwitch.ino

  // With batteryCheck true will return true only if the Agrumino is attached to USB with a valid power
  boolean resetWifi = checkIfResetWiFiSettings(true);
  boolean hasWifiCredentials = WiFi.SSID().length() > 0;

  if (resetWifi || !hasWifiCredentials) {
    // Show Configuration portal

    // Blink and keep ON led as a feedback :)
    blinkLed(100, 5);
    agrumino.turnLedOn();

    WiFiManager wifiManager;

    // Customize the web configuration web page
    wifiManager.setCustomHeadElement("<h1>Agrumino</h1>");

    // Sets timeout in seconds until configuration portal gets turned off.
    // If not specified device will remain in configuration mode until
    // switched off via webserver or device is restarted.
    // wifiManager.setConfigPortalTimeout(600);

    // Starts an access point and goes into a blocking loop awaiting configuration
    String ssidAP = "Agrumino-AP-" + getChipId();
    boolean gotConnection = wifiManager.startConfigPortal(ssidAP.c_str());

    Serial.print("\nGot connection from Config Portal: ");
    Serial.println(gotConnection);

    agrumino.turnLedOff();

    // ESP.reset() doesn't work on first reboot
    agrumino.deepSleepSec(1);
    return;

  } else {
    // Try to connect to saved network
    // Force to station mode because if device was switched off while in access point mode it will start up next time in access point mode.
    agrumino.turnLedOn();
    WiFi.mode(WIFI_STA);
    WiFi.waitForConnectResult();
  }

  boolean connected = (WiFi.status() == WL_CONNECTED);

  if (connected) {
    // If you get here you have connected to the WiFi :D
    Serial.print("\nConnected to WiFi as ");
    Serial.print(WiFi.localIP());
    Serial.println(" ...yeey :)\n");
  } else {
    // No WiFi connection. Skip a cycle and retry later
    Serial.print("\nNot connected!\n");
    // ESP.reset() doesn't work on first reboot
    agrumino.deepSleepSec(SLEEP_TIME_SEC);
  }
}

void loop() {
  Serial.println("#########################\n");

  agrumino.turnBoardOn();
  agrumino.turnLedOn();

  float temperature =         agrumino.readTempC();
  unsigned int soilMoisture = agrumino.readSoil();
  float illuminance =         agrumino.readLux();
  float batteryVoltage =      agrumino.readBatteryVoltage();
  unsigned int batteryLevel = agrumino.readBatteryLevel();
  boolean isAttachedToUSB =   agrumino.isAttachedToUSB();
  boolean isBatteryCharging = agrumino.isBatteryCharging();

  Serial.println("temperature:       " + String(temperature) + "°C");
  Serial.println("soilMoisture:      " + String(soilMoisture) + "%");
  Serial.println("illuminance :      " + String(illuminance) + " lux");
  Serial.println("batteryVoltage :   " + String(batteryVoltage) + " V");
  Serial.println("batteryLevel :     " + String(batteryLevel) + "%");
  Serial.println("isAttachedToUSB:   " + String(isAttachedToUSB));
  Serial.println("isBatteryCharging: " + String(isBatteryCharging));
  Serial.println();

  // Change this if you whant to change your thing name
  // We use the chip Id to avoid name clashing
  String dweetThingName = "Agrumino-" + getChipId();

  // Send data to our web service
  sendData(dweetThingName, temperature, soilMoisture, illuminance, batteryVoltage, batteryLevel, isAttachedToUSB, isBatteryCharging);

  // Blink when the business is done for giving an Ack to the user
  blinkLed(200, 2);

  // Board off before delay/sleep to save battery :)
  agrumino.turnBoardOff();

  // delaySec(SLEEP_TIME_SEC); // The ESP8266 stays powered, executes the loop repeatedly
  agrumino.deepSleepSec(SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time starts back from setup() and then loop()
}

//////////////////
// HTTP methods //
//////////////////

void sendData(String dweetName, float temp, int soil, unsigned int lux, float batt, unsigned int battLevel, boolean usb, boolean charge) {

  String bodyJsonString = getSendDataBodyJsonString(temp,  soil,  lux,  batt, battLevel, usb, charge );

  // Use WiFiClient class to create TCP connections, we try until the connection is estabilished
  while (!client.connect(WEB_SERVER_HOST, 80)) {
    Serial.println("connection failed\n");
    delay(1000);
  }
  Serial.println("connected to " + String(WEB_SERVER_HOST) + " ...yeey :)\n");

  Serial.println("###################################");
  Serial.println("### Your Dweet.io thing name is ###");
  Serial.println("###   --> " + dweetName + " <--  ###");
  Serial.println("###################################\n");

  // Print the HTTP POST API data for debug
  Serial.println("Requesting POST: " + String(WEB_SERVER_HOST) + WEB_SERVER_API_SEND_DATA + dweetName);
  Serial.println("Requesting POST: " + bodyJsonString);

  // This will send the request to the server
  client.println("POST " + WEB_SERVER_API_SEND_DATA + dweetName + " HTTP/1.1");
  client.println("Host: " + String(WEB_SERVER_HOST) + ":80");
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(bodyJsonString.length()));
  client.println();
  client.println(bodyJsonString);

  delay(10);

  int timeout = 300; // 100 ms per loop so 30 sec.
  while (!client.available()) {
    // Waiting for server response
    delay(100);
    Serial.print(".");
    timeout--;
    if (timeout <= 0) {
      Serial.print("Err. client.available() timeout reached!");
      return;
    }
  }

  String response = "";

  while (client.available() > 0) {
    char c = client.read();
    response = response + c;
  }

  // Remove bad chars from response
  response.trim();
  response.replace("/n", "");
  response.replace("/r", "");

  Serial.println("\nAPI Update successful! Response: \n");
  Serial.println(response);
}

// Returns the Json body that will be sent to the send data HTTP POST API
String getSendDataBodyJsonString(float temp, int soil, unsigned int lux, float batt, unsigned int battLevel, boolean usb, boolean charge) {
  JsonObject& jsonPost = jsonBuffer.createObject();
  jsonPost["temp"] = String(temp);
  jsonPost["soil"] = String(soil);
  jsonPost["lux"]  = String(lux);
  jsonPost["battVolt"] = String(batt);
  jsonPost["battLevel"] = String(battLevel);
  jsonPost["battCharging"] = String(charge);
  jsonPost["usbConnected"]  = String(usb);

  String jsonPostString;
  jsonPost.printTo(jsonPostString);

  return jsonPostString;
}


/////////////////////
// Utility methods //
/////////////////////

void blinkLed(int duration, int blinks) {
  for (int i = 0; i < blinks; i++) {
    agrumino.turnLedOn();
    delay(duration);
    agrumino.turnLedOff();
    if (i < blinks) {
      delay(duration); // Avoid delay in the latest loop ;)
    }
  }
}

void delaySec(int sec) {
  delay (sec * 1000);
}

const String getChipId() {
  // Returns the ESP Chip ID, Typical 7 digits
  return String(ESP.getChipId());
}

// If the Agrumino S1 button is pressed for 5 secs then reset the wifi saved settings.
// If "checkBattery" is true the method return true only if the USB is connected.
boolean checkIfResetWiFiSettings(boolean checkBattery) {
  int delayMs = 100;
  int remainingsLoops = (5 * 1000) / delayMs;
  Serial.print("\nCheck if reset WiFi settings: ");
  while (remainingsLoops > 0
         && agrumino.isButtonPressed()
         && (agrumino.isAttachedToUSB() || !checkBattery) // The Agrumino must be attached to USB
        ) {
    // Blink the led every sec as confirmation
    if (remainingsLoops % 10 == 0) {
      agrumino.turnLedOn();
    }
    delay(delayMs);
    agrumino.turnLedOff();
    remainingsLoops--;
    Serial.print(".");
  }
  agrumino.turnLedOff();
  boolean success = (remainingsLoops == 0);
  Serial.println(success ? " YES!" : " NO");
  Serial.println();
  return success;
}
