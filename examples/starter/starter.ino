// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Import libraries (BLEPeripheralObserver depends on SPI)
#include <SPI.h>
#include <BLEPeripheralObserver.h>

//custom boards may override default pin definitions with BLEPeripheralObserver(PIN_REQ, PIN_RDY, PIN_RST)
BLEPeripheralObserver                    blePeriphObserv                            = BLEPeripheralObserver();

// uuid's can be:
//   16-bit: "ffff"
//  128-bit: "19b10010e8f2537e4f6cd104768a1214" (dashed format also supported)

// create one or more services
BLEService service = BLEService("fff0");

// create one or more characteristics
BLECharCharacteristic characteristic = BLECharCharacteristic("fff1", BLERead | BLEWrite);

// create one or more descriptors (optional)
BLEDescriptor descriptor = BLEDescriptor("2901", "value");

void setup() {
  Serial.begin(115200);
#if defined (__AVR_ATmega32U4__)
  delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
#endif

  blePeriphObserv.setLocalName("local-name"); // optional
  blePeriphObserv.setAdvertisedServiceUuid(service.uuid()); // optional

  // add attributes (services, characteristics, descriptors) to peripheral
  blePeriphObserv.addAttribute(service);
  blePeriphObserv.addAttribute(characteristic);
  blePeriphObserv.addAttribute(descriptor);

  // set initial value
  characteristic.setValue(0);

  // begin initialization
  blePeriphObserv.begin();
}

void loop() {
  BLECentral central = blePeriphObserv.central();

  if (central) {
    // central connected to peripheral
    Serial.print(F("Connected to central: "));
    Serial.println(central.address());

    while (central.connected()) {
      // central still connected to peripheral
      if (characteristic.written()) {
        // central wrote new value to characteristic
        Serial.println(characteristic.value(), DEC);

        // set value on characteristic
        characteristic.setValue(5);
      }
    }

    // central disconnected
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
  }
}