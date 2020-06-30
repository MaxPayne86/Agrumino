/*
  AgruminoSample.ino - Sample project for Agrumino board using the Agrumino library.
  Created by giuseppe.broccia@lifely.cc on October 2017.

  @see Agrumino.h for the documentation of the lib
*/

#include <Agrumino.h>

#define SLEEP_TIME_SEC 1

Agrumino agrumino;

void setup() {
  Serial.begin(115200);
  agrumino.setup();

  int type = 0;

  switch (type) {
    case 0:
      // None, use default values
      break;
    case 1: // Resistive
      agrumino.calibrateSoilWater(360);
      agrumino.calibrateSoilAir(4000);
      break;
    case 2: // Capacitive
      agrumino.calibrateSoilWater(460);
      agrumino.calibrateSoilAir(3600);
      break;
    case 3: // DFRobot
      agrumino.calibrateSoilWater(1400);
      agrumino.calibrateSoilAir(2880);
      break;
    case 4: // Capacitive no holes
      agrumino.calibrateSoilWater(2200);
      agrumino.calibrateSoilAir(3400);
      break;
  }

  agrumino.turnBoardOn();
}

void loop() {
  Serial.println("#########################\n");

  boolean isAttachedToUSB =   agrumino.isAttachedToUSB();
  boolean isBatteryCharging = agrumino.isBatteryCharging();
  boolean isButtonPressed =   agrumino.isButtonPressed();
  float temperature =         agrumino.readTempC();
  unsigned int soilMoisture = agrumino.readSoil();
  unsigned int soilMoistureRaw = agrumino.readSoilRaw();
  float illuminance =         agrumino.readLux();
  float batteryVoltage =      agrumino.readBatteryVoltage();

  Serial.println("");
  Serial.println("isAttachedToUSB:   " + String(isAttachedToUSB));
  Serial.println("isBatteryCharging: " + String(isBatteryCharging));
  Serial.println("isButtonPressed:   " + String(isButtonPressed));
  Serial.println("temperature:       " + String(temperature) + "°C");
  Serial.println("soilMoisture:      " + String(soilMoisture) + "%");
  Serial.println("soilMoistureRaw:   " + String(soilMoistureRaw));
  Serial.println("illuminance :      " + String(illuminance) + " lux");
  Serial.println("batteryVoltage :   " + String(batteryVoltage) + " V");
  Serial.println("");

  if (isButtonPressed) {
    agrumino.turnWateringOn();
    // delay(2000);
  } else {
    agrumino.turnWateringOff();
  }

  blinkLed();

  // Blink GPIO_1 or GPIO_2 based on soil
  if (soilMoisture > 66) {
    blinkGPIO(LIV_1);
  } else if (soilMoisture > 33) {
    blinkGPIO(GPIO_2);
  } else {
    blinkGPIO(GPIO_1);
  }

  // agrumino.turnBoardOff(); // Board off before delay/sleep to save battery :)

  delaySec(SLEEP_TIME_SEC); // The ESP8266 stays powered, executes the loop repeatedly
  // deepSleepSec(SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time starts back from setup() and then loop()
}

/////////////////////
// Utility methods //
/////////////////////

void blinkLed() {
  agrumino.turnLedOn();
  delay(200);
  agrumino.turnLedOff();
}

void blinkGPIO(gpio_pin pin) {
  agrumino.setGPIOMode(pin, GPIO_OUTPUT);
  agrumino.writeGPIO(pin, HIGH);
  delay(200);
  agrumino.writeGPIO(pin, LOW);
  delay(200);
}

void delaySec(int sec) {
  delay (sec * 1000);
}
