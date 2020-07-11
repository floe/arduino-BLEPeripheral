// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Import libraries (BLEPeripheralObserver depends on SPI)
#include <SPI.h>
#include <BLEPeripheralObserver.h>

// LED pin
#define LED_PIN   3

//custom boards may override default pin definitions with BLEPeripheralObserver(PIN_REQ, PIN_RDY, PIN_RST)
BLEPeripheralObserver                    blePeriphObserv                            = BLEPeripheralObserver();

// create service
BLEService              ledService           = BLEService("19b10000e8f2537e4f6cd104768a1214");

// create switch characteristic
BLECharCharacteristic   switchCharacteristic = BLECharCharacteristic("19b10001e8f2537e4f6cd104768a1214", BLERead | BLEWrite);

void setup() {
  Serial.begin(115200);
#if defined (__AVR_ATmega32U4__)
  delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
#endif

  // set LED pin to output mode
  pinMode(LED_PIN, OUTPUT);

  // set advertised local name and service UUID
  blePeriphObserv.setLocalName("LED");
  blePeriphObserv.setAdvertisedServiceUuid(ledService.uuid());

  // add service and characteristic
  blePeriphObserv.addAttribute(ledService);
  blePeriphObserv.addAttribute(switchCharacteristic);

  // assign event handlers for connected, disconnected to peripheral
  blePeriphObserv.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeriphObserv.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristic
  switchCharacteristic.setEventHandler(BLEWritten, switchCharacteristicWritten);

  // begin initialization
  blePeriphObserv.begin();

  Serial.println(F("BLE LED Peripheral"));
}

void loop() {
  // poll peripheral
  blePeriphObserv.poll();
}

void blePeripheralConnectHandler(BLECentral& central) {
  // central connected event handler
  Serial.print(F("Connected event, central: "));
  Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLECentral& central) {
  // central disconnected event handler
  Serial.print(F("Disconnected event, central: "));
  Serial.println(central.address());
}

void switchCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  // central wrote new value to characteristic, update LED
  Serial.print(F("Characteristic event, writen: "));

  if (switchCharacteristic.value()) {
    Serial.println(F("LED on"));
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println(F("LED off"));
    digitalWrite(LED_PIN, LOW);
  }
}
