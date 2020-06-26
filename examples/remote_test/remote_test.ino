// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Import libraries (BLEPeripheralObserver depends on SPI)
#include <SPI.h>
#include <BLEPeripheralObserver.h>
#include <BLEUtil.h>

//custom boards may override default pin definitions with BLEPeripheralObserver(PIN_REQ, PIN_RDY, PIN_RST)
BLEPeripheralObserver                    blePeriphObserv                            = BLEPeripheralObserver();

// create remote services
BLERemoteService                 remoteService                            = BLERemoteService("fffffffffffffffffffffffffffffff0");

// create remote characteristics
BLERemoteCharacteristic          remoteCharacteristic1                    = BLERemoteCharacteristic("fffffffffffffffffffffffffffffff1", BLERead);
BLERemoteCharacteristic          remoteCharacteristic2                    = BLERemoteCharacteristic("fffffffffffffffffffffffffffffff2", BLERead);
BLERemoteCharacteristic          remoteCharacteristic3                    = BLERemoteCharacteristic("fffffffffffffffffffffffffffffff3", BLEWrite | BLEWriteWithoutResponse);
BLERemoteCharacteristic          remoteCharacteristic4                    = BLERemoteCharacteristic("fffffffffffffffffffffffffffffff4", BLENotify);


void setup() {
  Serial.begin(9600);
#if defined (__AVR_ATmega32U4__)
  while(!Serial); // wait for serial
#endif

  blePeriphObserv.setLocalName("remote-test");

  // set device name and appearance
  blePeriphObserv.setDeviceName("Remote Test");
  blePeriphObserv.setAppearance(0x0080);

  blePeriphObserv.addRemoteAttribute(remoteService);
  blePeriphObserv.addRemoteAttribute(remoteCharacteristic1);
  blePeriphObserv.addRemoteAttribute(remoteCharacteristic2);
  blePeriphObserv.addRemoteAttribute(remoteCharacteristic3);
  blePeriphObserv.addRemoteAttribute(remoteCharacteristic4);

  // assign event handlers for connected, disconnected to peripheral
  blePeriphObserv.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeriphObserv.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
  blePeriphObserv.setEventHandler(BLERemoteServicesDiscovered, blePeripheralRemoteServicesDiscoveredHandler);

  // assign event handlers for characteristic
  remoteCharacteristic1.setEventHandler(BLEValueUpdated, bleRemoteCharacteristicValueUpdatedHandle);
  remoteCharacteristic2.setEventHandler(BLEValueUpdated, bleRemoteCharacteristicValueUpdatedHandle);
  remoteCharacteristic4.setEventHandler(BLEValueUpdated, bleRemoteCharacteristicValueUpdatedHandle);


  // begin initialization
  blePeriphObserv.begin();

  Serial.println(F("BLE Peripheral - remote test"));
}

void loop() {
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

void blePeripheralRemoteServicesDiscoveredHandler(BLECentral& central) {
  // central remote services discovered event handler
  Serial.print(F("Remote services discovered event, central: "));
  Serial.println(central.address());

//  if (remoteCharacteristic1.canRead()) {
//    Serial.println(F("Read 1"));
//    remoteCharacteristic1.read();
//  }
//
//  if (remoteCharacteristic2.canRead()) {
//    Serial.println(F("Read 2"));
//    remoteCharacteristic2.read();
//  }
//
//  if (remoteCharacteristic3.canWrite()) {
//    Serial.println(F("Write 3"));
//
//    unsigned long writeValue = 42;
//
//    remoteCharacteristic3.write((const unsigned char*)&writeValue, sizeof(writeValue));
//  }
//
//  if (remoteCharacteristic4.canSubscribe()) {
//    Serial.println(F("Subscribe 4"));
//
//    remoteCharacteristic4.subscribe();
//  }
}

void bleRemoteCharacteristicValueUpdatedHandle(BLECentral& central, BLERemoteCharacteristic& characteristic) {
  Serial.print(F("Remote characteristic value: "));

  BLEUtil::printBuffer(characteristic.value(), characteristic.valueLength());
}

