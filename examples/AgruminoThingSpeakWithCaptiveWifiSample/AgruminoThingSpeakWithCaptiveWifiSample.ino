/*
  AgruminoThingSpeakWithCaptiveWifixxx.ino
  Created by Paolo Paolucci on May 2018.
*/

#include "Agrumino.h"           // Our super cool lib ;)
#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <DNSServer.h>          // Installed from ESP8266 board
#include <ESP8266WebServer.h>   // Installed from ESP8266 board
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson

// Time to sleep in second between the readings/data sending
#define SLEEP_TIME_SEC 60

// ThingSpeak information.
#define NUM_FIELDS 5                               // To update more fields, increase this number and add a field label below.
#define TEMPERATURA 2
#define UMIDITA 1
#define LUMINOSITA 3
#define TENSIONE 4
#define LIVELLOBATT 5
#define THING_SPEAK_ADDRESS "api.thingspeak.com"
// String writeAPIKey="XXXXXXXXXXXXXXXX";             // Change this to your channel Write API key.
#define TIMEOUT  5000                              // Timeout for server response.

// Our super cool lib
Agrumino agrumino;

// Used for sending Json POST requests
StaticJsonBuffer<500> jsonBuffer;
// Used to create TCP connections and make Http calls
WiFiClient client;

void setup() {
  Serial.begin(115200);

  // Setup our super cool lib
  agrumino.setup();

  // Turn on the board to allow the usage of the Led
  agrumino.turnBoardOn();

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // If the S1 button is pressed for 5 seconds then reset the wifi saved settings.
  if (checkIfResetWiFiSettings()) {
    wifiManager.resetSettings();
    Serial.println("Reset WiFi Settings Done!");
    // Blink led for confirmation :)
    blinkLed(100, 10);
  }

  // Set timeout until configuration portal gets turned off
  // useful to make it all retry or go to sleep in seconds
  wifiManager.setTimeout(180); // 3 minutes

  // Customize the web configuration web page
  wifiManager.setCustomHeadElement("<h1>Agrumino</h1>");

  // Fetches ssid and pass and tries to connect
  // If it does not connect it starts an access point with the specified name here
  // and goes into a blocking loop awaiting configuration
  String ssidAP = "Agrumino-AP-" + getChipId();
  if (!wifiManager.autoConnect(ssidAP.c_str())) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    // Reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  // If you get here you have connected to the WiFi :D
  Serial.println("\nConnected to WiFi ...yeey :)\n");
}

void loop() {
  Serial.println("#########################\n");

  agrumino.turnBoardOn();

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
  String thingName = "Agrumino-" + getChipId();

  // You can fill fieldData with up to 8 values to write to successive fields in your channel.
  String fieldData[ NUM_FIELDS + 1 ];

  // You can write to multiple fields by storing data in the fieldData[] array, and changing numFields.
  // Write the moisture data to field 1.
  fieldData[ UMIDITA ] = String(soilMoisture);
  fieldData[ TEMPERATURA ] = String(temperature);
  fieldData[ LUMINOSITA ] = String(illuminance);
  fieldData[ TENSIONE ] = String(batteryVoltage);
  fieldData[ LIVELLOBATT ] = String(batteryLevel);

  HTTPPost( NUM_FIELDS , fieldData );

  // Blink when the business is done for giving an Ack to the user
  blinkLed(200, 1);

  // Board off before delay/sleep to save battery :)
  agrumino.turnBoardOff();

  // delaySec(SLEEP_TIME_SEC); // The ESP8266 stays powered, executes the loop repeatedly
  deepSleepSec(SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time starts back from setup() and then loop()
}

//////////////////
// HTTP methods //
//////////////////

int HTTPPost( int numFields , String fieldData[] ) {
  if (client.connect( THING_SPEAK_ADDRESS , 80 )) {
    // Build the Posting data string.
    // If you have multiple fields, make sure the sting does not exceed 1440 characters.
    String postData = "api_key=" + writeAPIKey ;
    for ( int fieldNumber = 1; fieldNumber < numFields + 1 ; fieldNumber++ ) {
      String fieldName = "field" + String( fieldNumber );
      postData += "&" + fieldName + "=" + fieldData[ fieldNumber ];
    }
    // POST data via HTTP
    Serial.println( "Connecting to ThingSpeak for update..." );
    Serial.println();
    client.println( "POST /update HTTP/1.1" );
    client.println( "Host: api.thingspeak.com" );
    client.println( "Connection: close" );
    client.println( "Content-Type: application/x-www-form-urlencoded" );
    client.println( "Content-Length: " + String( postData.length() ) );
    client.println();
    client.println( postData );
    Serial.println( postData );
    String answer = getResponse();
    Serial.println( answer );
  }
  else
  {
    Serial.println ( "Connection Failed" );
  }
}

// Wait for a response from the server to be available,
//and then collect the response and build it into a string.
String getResponse() {
  String response;
  long startTime = millis();
  delay( 200 );
  while ( client.available() < 1 && (( millis() - startTime ) < TIMEOUT ) ) {
    delay( 5 );
  }
  if ( client.available() > 0 ) { // Get response from server
    char charIn;
    do {
      charIn = client.read(); // Read a char from the buffer.
      response += charIn;     // Append the char to the string response.
    } while ( client.available() > 0 );
  }
  client.stop();
  return response;
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

void deepSleepSec(int sec) {
  Serial.print("\nGoing to deepSleep for ");
  Serial.print(sec);
  Serial.println(" seconds... (ー。ー) zzz\n");
  ESP.deepSleep(sec * 1000000); // microseconds
}

const String getChipId() {
  // Returns the ESP Chip ID, Typical 7 digits
  return String(ESP.getChipId());
}

// If the Agrumino S1 button is pressed for 5 seconds then reset the wifi saved settings.
boolean checkIfResetWiFiSettings() {

  int remainingsLoops = (5 * 1000) / 100;

  Serial.print("Check if reset WiFi settings: ");

  while (agrumino.isButtonPressed() && remainingsLoops > 0) {
    delay(100);
    remainingsLoops--;
    Serial.print(".");
  }

  if (remainingsLoops == 0) {
    // Reset Wifi Settings
    Serial.println(" YES!");
    return true;
  } else {
    Serial.println(" NO");
    return false;
  }
}

