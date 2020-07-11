// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// TimerOne library: https://code.google.com/p/arduino-timerone/
#include <TimerOne.h>
// DHT library: https://github.com/adafruit/DHT-sensor-library
#include "DHT.h"
#include <SPI.h>
#include <BLEPeripheralObserver.h>

#define DHTTYPE DHT22
#define DHTPIN 3

DHT dht(DHTPIN, DHTTYPE);

//custom boards may override default pin definitions with BLEPeripheralObserver(PIN_REQ, PIN_RDY, PIN_RST)
BLEPeripheralObserver                    blePeriphObserv                            = BLEPeripheralObserver();

BLEService tempService = BLEService("CCC0");
BLEFloatCharacteristic tempCharacteristic = BLEFloatCharacteristic("CCC1", BLERead | BLENotify);
BLEDescriptor tempDescriptor = BLEDescriptor("2901", "Temp Celsius");

BLEService humidityService = BLEService("DDD0");
BLEFloatCharacteristic humidityCharacteristic = BLEFloatCharacteristic("DDD1", BLERead | BLENotify);
BLEDescriptor humidityDescriptor = BLEDescriptor("2901", "Humidity Percent");

volatile bool readFromSensor = false;

float lastTempReading;
float lastHumidityReading;

void setup() {
  Serial.begin(115200);
#if defined (__AVR_ATmega32U4__)
  delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
#endif

  blePeriphObserv.setLocalName("Temperature");

  blePeriphObserv.setAdvertisedServiceUuid(tempService.uuid());
  blePeriphObserv.addAttribute(tempService);
  blePeriphObserv.addAttribute(tempCharacteristic);
  blePeriphObserv.addAttribute(tempDescriptor);

  blePeriphObserv.setAdvertisedServiceUuid(humidityService.uuid());
  blePeriphObserv.addAttribute(humidityService);
  blePeriphObserv.addAttribute(humidityCharacteristic);
  blePeriphObserv.addAttribute(humidityDescriptor);

  blePeriphObserv.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeriphObserv.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  blePeriphObserv.begin();

  Timer1.initialize(2 * 1000000); // in milliseconds
  Timer1.attachInterrupt(timerHandler);

  Serial.println(F("BLE Temperature Sensor Peripheral"));
}

void loop() {
  blePeriphObserv.poll();

  if (readFromSensor) {
    setTempCharacteristicValue();
    setHumidityCharacteristicValue();
    readFromSensor = false;
  }
}

void timerHandler() {
  readFromSensor = true;
}

void setTempCharacteristicValue() {
  float reading = dht.readTemperature();
//  float reading = random(100);

  if (!isnan(reading) && significantChange(lastTempReading, reading, 0.5)) {
    tempCharacteristic.setValue(reading);

    Serial.print(F("Temperature: ")); Serial.print(reading); Serial.println(F("C"));

    lastTempReading = reading;
  }
}

void setHumidityCharacteristicValue() {
  float reading = dht.readHumidity();
//  float reading = random(100);

  if (!isnan(reading) && significantChange(lastHumidityReading, reading, 1.0)) {
    humidityCharacteristic.setValue(reading);

    Serial.print(F("Humidity: ")); Serial.print(reading); Serial.println(F("%"));

    lastHumidityReading = reading;
  }
}

boolean significantChange(float val1, float val2, float threshold) {
  return (abs(val1 - val2) >= threshold);
}

void blePeripheralConnectHandler(BLECentral& central) {
  Serial.print(F("Connected event, central: "));
  Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLECentral& central) {
  Serial.print(F("Disconnected event, central: "));
  Serial.println(central.address());
}

