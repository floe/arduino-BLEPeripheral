// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define NRF51
#undef __RFduino__
// Import libraries (BLEPeripheralObserver depends on SPI)
#include <SPI.h>
#include <BLEPeripheralObserver.h>
#include <BLEUtil.h>
#include <ble_gap.h>

//custom boards may override default pin definitions with BLEPeripheralObserver(PIN_REQ, PIN_RDY, PIN_RST)
BLEPeripheralObserver                    blePeriphObserv                            = BLEPeripheralObserver();

void setup() {
  Serial.begin(115200);
#if defined (__AVR_ATmega32U4__)
  delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
#endif

  blePeriphObserv.setLocalName("foobaz"); // optional
  
  blePeriphObserv.setEventHandler(BLEAddressReceived, addrHandler);
  blePeriphObserv.setEventHandler(BLEAdvertisementReceived, advHandler);

  // begin initialization
  blePeriphObserv.begin();
  blePeriphObserv.setConnectable(false);
  blePeriphObserv.setAdvertisingInterval(500);
  blePeriphObserv.startAdvertising();
}

void addrHandler(const void* _addr) {
  unsigned char* addr = (unsigned char*)_addr;
  char address[18];
  BLEUtil::addressToString(addr, address);
  Serial.print(F("Got own addr: "));
  Serial.println(address);
}

void advHandler(const void* adv) {
  ble_gap_evt_adv_report_t* report = (ble_gap_evt_adv_report_t*)adv;
  char address[18];
  BLEUtil::addressToString(report->peer_addr.addr, address);
  Serial.print(F("Evt Adv Report from "));
  Serial.println(address);
  Serial.print(F("got adv with payload "));
  Serial.println(report->dlen);
  Serial.print(F("RSSI "));
  Serial.println(report->rssi);
}

void loop() {
  if (Serial.available() > 0) {
    Serial.read();
    Serial.println("start scanning");
    blePeriphObserv.startScanning();
    blePeriphObserv.setLocalName("foobarg"); // optional
    blePeriphObserv.startAdvertising();
  }
  blePeriphObserv.poll();
}
