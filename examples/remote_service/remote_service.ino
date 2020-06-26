// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Import libraries (BLEPeripheralObserver depends on SPI)
#include <SPI.h>
#include <BLEPeripheralObserver.h>

//custom boards may override default pin definitions with BLEPeripheralObserver(PIN_REQ, PIN_RDY, PIN_RST)
BLEPeripheralObserver                    blePeriphObserv                            = BLEPeripheralObserver();

// create remote services
BLERemoteService                 remoteGenericAttributeService            = BLERemoteService("1800");

// create remote characteristics
BLERemoteCharacteristic          remoteDeviceNameCharacteristic           = BLERemoteCharacteristic("2a00", BLERead);


void setup() {
  Serial.begin(9600);
#if defined (__AVR_ATmega32U4__)
  while(!Serial); // wait for serial
#endif

  blePeriphObserv.setLocalName("remote-attributes");

  // set device name and appearance
  blePeriphObserv.setDeviceName("Remote Attributes");
  blePeriphObserv.setAppearance(0x0080);

  blePeriphObserv.addRemoteAttribute(remoteGenericAttributeService);
  blePeriphObserv.addRemoteAttribute(remoteDeviceNameCharacteristic);

  // assign event handlers for connected, disconnected to peripheral
  blePeriphObserv.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeriphObserv.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
  blePeriphObserv.setEventHandler(BLERemoteServicesDiscovered, blePeripheralRemoteServicesDiscoveredHandler);

  // assign event handlers for characteristic
  remoteDeviceNameCharacteristic.setEventHandler(BLEValueUpdated, bleRemoteDeviceNameCharacteristicValueUpdatedHandle);

  // begin initialization
  blePeriphObserv.begin();

  Serial.println(F("BLE Peripheral - remote attributes"));
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

  if (remoteDeviceNameCharacteristic.canRead()) {
    remoteDeviceNameCharacteristic.read();
  }
}

void bleRemoteDeviceNameCharacteristicValueUpdatedHandle(BLECentral& central, BLERemoteCharacteristic& characteristic) {
  char remoteDeviceName[BLE_REMOTE_ATTRIBUTE_MAX_VALUE_LENGTH + 1];
  memset(remoteDeviceName, 0, sizeof(remoteDeviceName));
  memcpy(remoteDeviceName, remoteDeviceNameCharacteristic.value(), remoteDeviceNameCharacteristic.valueLength());

  Serial.print(F("Remote device name: "));
  Serial.println(remoteDeviceName);
}

