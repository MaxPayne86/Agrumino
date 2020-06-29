/*
  AgruminoCaptiveWiSample.ino - Sample project for Agrumino board using the Agrumino library.
  !!!WARNING!!! You need to program the board with option Tools->Erase Flash->All Flash Contents

  Created by giuseppe.broccia@lifely.cc on October 2017.
  Modified June 2020 by:
  Massimo Pennazio massimo.pennazio@abinsula.com
  Martina Mascia martina.mascia@abinsula.com
  Ricardo Medda ricardo.medda@abinsula.com

  @see Agrumino.h for the documentation of the lib
*/
#include "Agrumino.h"           // Our super cool lib ;)
#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <DNSServer.h>          // Installed from ESP8266 board
//#include <ESP8266WebServer.h>   // Installed from ESP8266 board
//#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson

//  ***start AsyncElegantOTA 
#include <ESPAsyncWiFiManager.h>  //https://github.com/alanswx/ESPAsyncWiFiManager
#include <Hash.h>
#include <ESPAsyncTCP.h>          //https://github.com/me-no-dev/ESPAsyncTCP
#include <ESPAsyncWebServer.h>    //https://github.com/me-no-dev/ESPAsyncWebServer
#include <AsyncElegantOTA.h>      //installed from Manages Libraries

AsyncWebServer server(80);
DNSServer dns;

#include <ESP8266mDNS.h>;
//  ***end AsyncElegantOTA

String myLocalIP = "";
String deviceNameMDNS = "";

// Time to sleep in second between the readings/data sending
#define SLEEP_TIME_SEC 3600 // [Seconds]

// The size of the flash sector we want to use to store samples (do not modify).
#define SECTOR_SIZE 4096u

// Web Server data, in our sample we use Dweet.io.
const char* WEB_SERVER_HOST = "dweet.io";
const String WEB_SERVER_API_SEND_DATA = "/dweet/quietly/for/"; // The Dweet name is created in the loop() method.


#define PUSH_1   4
uint8_t push_1_lock = 0;
uint32_t timec=0, prevtimec=0;


// Our super cool lib
Agrumino agrumino;

// Used for sending Json POST requests
//StaticJsonBuffer<200 * N_SAMPLES> jsonBuffer;
StaticJsonBuffer<200> jsonBuffer;

// Used to create TCP connections and make Http calls
WiFiClient client;

flashMemory_t *PtrFlashMemory = NULL;

uint32_t crc32b = 0;
uint16_t currentIndex = 0;
uint8_t crc8b = 0;

////////////////////////////////////////
// Utility methods function prototypes//
////////////////////////////////////////
boolean checkIfResetWiFiSettings(boolean checkBattery);
void delaySec(int sec);
const String getChipId();
uint32_t calculateCRC32(const uint8_t *data, size_t length);
void cleanMemory();
void blinkLed(int duration, int blinks);


////////////////////////////////////////
// HTTP methods function prototypes/////
////////////////////////////////////////
String getSendDataBodyJsonString(float temp, int soil, float lux, float batt, unsigned int battLevel, boolean usb, boolean charge);
void sendData(String dweetName, float temp, int soil, float lux, float batt, unsigned int battLevel, boolean usb, boolean charge);


void setup() {

  Serial.begin(115200);

  // Setup our super cool lib
  agrumino.setup();

  // Turn on the board to allow the usage of the Led
  agrumino.turnBoardOn();

  pinMode(PUSH_1, INPUT_PULLUP);

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

//    WiFiManager wifiManager;

//  AsyncElegantOTA  INIZIO
    AsyncWiFiManager wifiManager(&server,&dns);
//  AsyncElegantOTA  FINE

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
    myLocalIP = WiFi.localIP().toString();
    Serial.print(WiFi.localIP());
    Serial.println(" ...yeey :)\n");
  } else {
    // No WiFi connection. Skip a cycle and retry later
    Serial.print("\nNot connected!\n");
    // ESP.reset() doesn't work on first reboot
    agrumino.deepSleepSec(SLEEP_TIME_SEC);
  }



  deviceNameMDNS = "a" + String(getChipId().toInt(), HEX);  // I turn the string into hexadecimal to shorten it

 // Serial.println("-----------> " + String(getChipId().toInt(), HEX));

  
  // To use MDNS please install avahi on linux, bonjour on windows   a3398435  a33db23
  if (!MDNS.begin(deviceNameMDNS)) {  
    Serial.println("Error setting up MDNS responder!");  
  }
  else {  
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS responder started");  
  }
  
//  AsyncElegantOTA  INIZIO
  Serial.println("Starting HTTP server.....");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "<html lang=\"en\">  <head><meta charset=\"utf-8\"> <title> The Agrumino summary page </title> </head><body><h1> Hi! I am your <i> Agrumino. </i> </h1></br>My local IP is: " + myLocalIP + "</br></br>My MDNS name is: " + deviceNameMDNS + ".local</br></br>My update page is: http://" + deviceNameMDNS + ".local/update (or http://" + myLocalIP + "/update)  </body></html>");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
//  AsyncElegantOTA  FINE

  

  // Initialize EEPROM and pointer
  Serial.println("Initializing EEPROM...");
  EEPROM.begin(SECTOR_SIZE);

  PtrFlashMemory = (flashMemory_t *)EEPROM.getDataPtr(); // Assigning pointer address to flash memory block dumped in RAM
  Serial.println("Memory assignement successful!");

  // Calculate checksum and validate memory area
  // TODO: use 32bit crc
  crc32b = calculateCRC32(PtrFlashMemory->Bytes, sizeof(Fields_t));
  crc8b = EEPROM.read(SECTOR_SIZE - 1); // Read crc at the end of sector
  Serial.println("CRC32 calculated=" + String(crc32b & 0xff) + " readed=" + String(crc8b));

  if (((uint8_t)crc32b & 0xff) == crc8b)
    Serial.println("CRC32 pass!");
  else
  {
    Serial.println("CRC32 fail! Cleaning memory...");
    cleanMemory();
  }
}

void loop() {
  Serial.println("\n#########################\n");
  Serial.println("My local IP is: " + myLocalIP);  
  Serial.println("My MDNS name is: " + deviceNameMDNS + ".local");  
  Serial.println("My update page is: " + deviceNameMDNS + ".local/update (or " + myLocalIP + "/update)");
  Serial.println("\n\n");

  MDNS.update();

//  AsyncElegantOTA  INIZIO  
  AsyncElegantOTA.loop();
//  AsyncElegantOTA  FINE
  
  agrumino.turnBoardOn();
  agrumino.turnLedOn();

  // Variable currentIndex is the last memorized struct of sensor data
  currentIndex = PtrFlashMemory->Fields.index;
  if (currentIndex > N_SAMPLES - 1) { // Circular buffer behaviour
    currentIndex = 0;
    PtrFlashMemory->Fields.index = currentIndex;
  }

  Serial.println("*****   CURRENT INDEX IS:  " + String(currentIndex));

  // Copy sensors data to struct
  PtrFlashMemory->Fields.data.vector[currentIndex].temp = agrumino.readTempC();
  PtrFlashMemory->Fields.data.vector[currentIndex].soil = agrumino.readSoil();
  PtrFlashMemory->Fields.data.vector[currentIndex].lux = agrumino.readLux();
  PtrFlashMemory->Fields.data.vector[currentIndex].batt = agrumino.readBatteryVoltage();
  PtrFlashMemory->Fields.data.vector[currentIndex].battLevel = agrumino.readBatteryLevel();
  PtrFlashMemory->Fields.data.vector[currentIndex].usb = agrumino.isAttachedToUSB();
  PtrFlashMemory->Fields.data.vector[currentIndex].charge = agrumino.isBatteryCharging();

  PtrFlashMemory->Fields.index++; // Increment index

  Serial.println("temperature:       " + String(PtrFlashMemory->Fields.data.vector[currentIndex].temp) + "°C");
  Serial.println("soilMoisture:      " + String(PtrFlashMemory->Fields.data.vector[currentIndex].soil) + "%");
  Serial.println("illuminance :      " + String(PtrFlashMemory->Fields.data.vector[currentIndex].lux) + " lux");
  Serial.println("batteryVoltage :   " + String(PtrFlashMemory->Fields.data.vector[currentIndex].batt) + " V");
  Serial.println("batteryLevel :     " + String(PtrFlashMemory->Fields.data.vector[currentIndex].battLevel) + "%");
  Serial.println("isAttachedToUSB:   " + String(PtrFlashMemory->Fields.data.vector[currentIndex].usb));
  Serial.println("isBatteryCharging: " + String(PtrFlashMemory->Fields.data.vector[currentIndex].charge));
  Serial.println();

  // Calculate checksum
  crc32b = calculateCRC32(PtrFlashMemory->Bytes, sizeof(Fields_t));
  crc8b = (uint8_t)crc32b & 0xff;
  Serial.println("New CRC32=" + String(crc8b));
  EEPROM.write(SECTOR_SIZE - 1, crc8b); // Put crc at the end of sector

  // With EEPROM.commit() we write all our data from RAM
  // to flash in one block. Actually the entire sector is written (#SECTOR_SIZE bytes).
  // Byte-level access to ESP's flash is not possible with this flash chip.
  if (EEPROM.commit()) {
    Serial.println("EEPROM successfully committed");
  } else {
    Serial.println("ERROR! EEPROM commit failed");
  }

  timec = millis();
  if(timec-prevtimec >= 250)  // Here we manage button every 250ms
  { 

  if(digitalRead(PUSH_1)==LOW)
  {
    delay(50);
    if(digitalRead(PUSH_1)==LOW)
    {
      if(push_1_lock != 1)
      {
        push_1_lock = 1;
        // qui fa qualcosa
        Serial.println("-------> TASTO PREMUTO");

      }
    }
  }
  else
  {
    Serial.println("-------> TASTO NON PREMUTO");
    //push_1_lock = 0;
  }

  prevtimec = timec;
  } // End if 1000ms tick
  

  if(push_1_lock == 1) {
    Serial.println("....................................UPDATE MODE");
  } else {
    Serial.println("....................................NORMAL MODE");
  }





  // We have the queue full, we need to consume data and send to cloud.
  // Variable currentIndex will be resetted @next loop.
  if ((currentIndex == N_SAMPLES - 1) && !push_1_lock) {
    // Change this if you whant to change your thing name
    // We use the chip Id to avoid name clashing
    String dweetThingName = "Agrumino-" + getChipId();

    Serial.println("Now I'm sending " + String(N_SAMPLES) + " json(s) to Dweet");
    for (uint8_t i = 0; i < N_SAMPLES; i++) {
      // Send data to our web service
      Serial.println("Sending json n° " + String(i + 1) + " to Dweet");
      sendData(dweetThingName, PtrFlashMemory->Fields.data.vector[i].temp, PtrFlashMemory->Fields.data.vector[i].soil, PtrFlashMemory->Fields.data.vector[i].lux, PtrFlashMemory->Fields.data.vector[i].batt, PtrFlashMemory->Fields.data.vector[i].battLevel, PtrFlashMemory->Fields.data.vector[i].usb, PtrFlashMemory->Fields.data.vector[i].charge);
      delay(5000);
    }
  }

  // Blink when the business is done for giving an Ack to the user
  blinkLed(200, 2);

  // Board off before delay/sleep to save battery :)
  agrumino.turnBoardOff();






  if (!push_1_lock) {
    //delaySec(SLEEP_TIME_SEC); // The ESP8266 stays powered, executes the loop repeatedly
    agrumino.deepSleepSec(SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time starts back from setup() and then loop()   
  }
  


}

//////////////////
// HTTP methods //
//////////////////

void sendData(String dweetName, float temp, int soil, float lux, float batt, unsigned int battLevel, boolean usb, boolean charge) {

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
String getSendDataBodyJsonString(float temp, int soil, float lux, float batt, unsigned int battLevel, boolean usb, boolean charge) {
  jsonBuffer.clear();
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

// Function to calculate CRC32
// TODO: verify support for hardware CRC32 in bsp, eventually
// copy implementation from official Espressif's SDK if present.
uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffff;
  int i;
  for (i = 0; i < length; i++)
  {
    uint8_t c = data[i];
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}

// This function initilizes content of
// the data struct in RAM
void cleanMemory() {
  Serial.println("RAM memory data struct initialization");
  PtrFlashMemory->Fields.index = 0;
  for (uint8_t i = 0; i < N_SAMPLES; i++) {
    PtrFlashMemory->Fields.data.vector[i].temp = 0.0f;
    PtrFlashMemory->Fields.data.vector[i].soil = 0u;
    PtrFlashMemory->Fields.data.vector[i].lux = 0.0f;
    PtrFlashMemory->Fields.data.vector[i].batt = 0.0f;
    PtrFlashMemory->Fields.data.vector[i].battLevel = 0u;
    PtrFlashMemory->Fields.data.vector[i].usb = false;
    PtrFlashMemory->Fields.data.vector[i].charge = false;
  }
}
