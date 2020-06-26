// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "BLEUuid.h"

#include "BLEDeviceLimits.h"
#include "BLEUtil.h"

#include "BLEPeripheralObserver.h"

//#define BLE_PERIPHERAL_DEBUG

#define DEFAULT_DEVICE_NAME "Arduino"
#define DEFAULT_APPEARANCE  0x0000

BLEPeripheralObserver::BLEPeripheralObserver(unsigned char req, unsigned char rdy, unsigned char rst) :
#if defined(NRF51) || defined(NRF52) || defined(__RFduino__)
  _nRF51822(),
#else
  _nRF8001(req, rdy, rst),
#endif

  _advertisedServiceUuid(NULL),
  _serviceSolicitationUuid(NULL),
  _manufacturerData(NULL),
  _manufacturerDataLength(0),
  _localName(NULL),

  _localAttributes(NULL),
  _numLocalAttributes(0),
  _remoteAttributes(NULL),
  _numRemoteAttributes(0),

  _genericAccessService("1800"),
  _deviceNameCharacteristic("2a00", BLERead, 19),
  _appearanceCharacteristic("2a01", BLERead, 2),
  _genericAttributeService("1801"),
  _servicesChangedCharacteristic("2a05", BLEIndicate, 4),

  _remoteGenericAttributeService("1801"),
  _remoteServicesChangedCharacteristic("2a05", BLEIndicate),

  _central(this)
{
#if defined(NRF51) || defined(NRF52) || defined(__RFduino__)
  this->_device = &this->_nRF51822;
#else
  this->_device = &this->_nRF8001;
#endif

  memset(this->_eventHandlers, 0x00, sizeof(this->_eventHandlers));
  memset(this->_deviceEvents,  0x00, sizeof(this->_deviceEvents));

  this->setDeviceName(DEFAULT_DEVICE_NAME);
  this->setAppearance(DEFAULT_APPEARANCE);

  this->_device->setEventListener(this);
}

BLEPeripheralObserver::~BLEPeripheralObserver() {
  this->end();

  if (this->_remoteAttributes) {
    free(this->_remoteAttributes);
  }

  if (this->_localAttributes) {
    free(this->_localAttributes);
  }
}

unsigned char BLEPeripheralObserver::updateAdvertismentData() {
  unsigned char advertisementDataSize = 0;

  scanData.length = 0;

  unsigned char remainingAdvertisementDataLength = BLE_ADVERTISEMENT_DATA_MAX_VALUE_LENGTH + 2;
  if (this->_serviceSolicitationUuid){
    BLEUuid serviceSolicitationUuid = BLEUuid(this->_serviceSolicitationUuid);

    unsigned char uuidLength = serviceSolicitationUuid.length();
    advertisementData[advertisementDataSize].length = uuidLength;
    advertisementData[advertisementDataSize].type = (uuidLength > 2) ? 0x15 : 0x14;

    memcpy(advertisementData[advertisementDataSize].data, serviceSolicitationUuid.data(), uuidLength);
    advertisementDataSize += 1;
    remainingAdvertisementDataLength -= uuidLength + 2;
  }
  if (this->_advertisedServiceUuid){
    BLEUuid advertisedServiceUuid = BLEUuid(this->_advertisedServiceUuid);

    unsigned char uuidLength = advertisedServiceUuid.length();
    if (uuidLength + 2 <= remainingAdvertisementDataLength) {
      advertisementData[advertisementDataSize].length = uuidLength;
      advertisementData[advertisementDataSize].type = (uuidLength > 2) ? 0x06 : 0x02;

      memcpy(advertisementData[advertisementDataSize].data, advertisedServiceUuid.data(), uuidLength);
      advertisementDataSize += 1;
      remainingAdvertisementDataLength -= uuidLength + 2;
    }
  }
  if (this->_manufacturerData && this->_manufacturerDataLength > 0) {
    if (remainingAdvertisementDataLength >= 3) {
      unsigned char dataLength = this->_manufacturerDataLength;

      if (dataLength + 2 > remainingAdvertisementDataLength) {
        dataLength = remainingAdvertisementDataLength - 2;
      }

      advertisementData[advertisementDataSize].length = dataLength;
      advertisementData[advertisementDataSize].type = 0xff;

      memcpy(advertisementData[advertisementDataSize].data, this->_manufacturerData, dataLength);
      advertisementDataSize += 1;
      remainingAdvertisementDataLength -= dataLength + 2;
    }
  }

  if (this->_localName){
    unsigned char localNameLength = strlen(this->_localName);
    scanData.length = localNameLength;

    if (scanData.length > BLE_SCAN_DATA_MAX_VALUE_LENGTH) {
      scanData.length = BLE_SCAN_DATA_MAX_VALUE_LENGTH;
    }

    scanData.type = (localNameLength > scanData.length) ? 0x08 : 0x09;

    memcpy(scanData.data, this->_localName, scanData.length);
  }

  return advertisementDataSize;
}

void BLEPeripheralObserver::begin() {

  if (this->_localAttributes == NULL) {
    this->initLocalAttributes();
  }

  for (int i = 0; i < this->_numLocalAttributes; i++) {
    BLELocalAttribute* localAttribute = this->_localAttributes[i];
    if (localAttribute->type() == BLETypeCharacteristic) {
      BLECharacteristic* characteristic = (BLECharacteristic*)localAttribute;

      characteristic->setValueChangeListener(*this);
    }
  }

  for (int i = 0; i < this->_numRemoteAttributes; i++) {
    BLERemoteAttribute* remoteAttribute = this->_remoteAttributes[i];
    if (remoteAttribute->type() == BLETypeCharacteristic) {
      BLERemoteCharacteristic* remoteCharacteristic = (BLERemoteCharacteristic*)remoteAttribute;

      remoteCharacteristic->setValueChangeListener(*this);
    }
  }

  if (this->_numRemoteAttributes) {
    this->addRemoteAttribute(this->_remoteGenericAttributeService);
    this->addRemoteAttribute(this->_remoteServicesChangedCharacteristic);
  }

  int advertisementDataSize = updateAdvertismentData();
  this->_device->begin(advertisementDataSize, advertisementData,
                        scanData.length > 0 ? 1 : 0, &scanData,
                        this->_localAttributes, this->_numLocalAttributes,
                        this->_remoteAttributes, this->_numRemoteAttributes);

  this->_device->requestAddress();
}

void BLEPeripheralObserver::poll() {
  this->_device->poll();
}

void BLEPeripheralObserver::end() {
  this->_device->end();
}

void BLEPeripheralObserver::setAdvertisedServiceUuid(const char* advertisedServiceUuid) {
  this->_advertisedServiceUuid = advertisedServiceUuid;
}

void BLEPeripheralObserver::setServiceSolicitationUuid(const char* serviceSolicitationUuid) {
  this->_serviceSolicitationUuid = serviceSolicitationUuid;
}

void BLEPeripheralObserver::setManufacturerData(const unsigned char manufacturerData[], unsigned char manufacturerDataLength) {
  this->_manufacturerData = manufacturerData;
  this->_manufacturerDataLength = manufacturerDataLength;
}

void BLEPeripheralObserver::setLocalName(const char* localName) {
  this->_localName = localName;
}

void BLEPeripheralObserver::setConnectable(bool connectable) {
  this->_device->setConnectable(connectable);
}

bool  BLEPeripheralObserver::setTxPower(int txPower) {
  return this->_device->setTxPower(txPower);
}

void BLEPeripheralObserver::setBondStore(BLEBondStore& bondStore) {
  this->_device->setBondStore(bondStore);
}

void BLEPeripheralObserver::startAdvertising() {
  int advertisementDataSize = updateAdvertismentData();
  this->_device->updateAdvertisementData(
      advertisementDataSize, advertisementData,
      scanData.length > 0 ? 1 : 0, &scanData);
  this->_device->startAdvertising();
}

void BLEPeripheralObserver::stopAdvertising() {
  this->_device->stopAdvertising();
}

void BLEPeripheralObserver::startScanning() {
  this->_device->startScanning();
}

void BLEPeripheralObserver::stopScanning() {
  this->_device->stopScanning();
}

void BLEPeripheralObserver::setDeviceName(const char* deviceName) {
  this->_deviceNameCharacteristic.setValue(deviceName);
}

void BLEPeripheralObserver::setAppearance(unsigned short appearance) {
  this->_appearanceCharacteristic.setValue((unsigned char *)&appearance, sizeof(appearance));
}

void BLEPeripheralObserver::addAttribute(BLELocalAttribute& attribute) {
  this->addLocalAttribute(attribute);
}

void BLEPeripheralObserver::addLocalAttribute(BLELocalAttribute& localAttribute) {
  if (this->_localAttributes == NULL) {
    this->initLocalAttributes();
  }

  this->_localAttributes[this->_numLocalAttributes] = &localAttribute;
  this->_numLocalAttributes++;
}

void BLEPeripheralObserver::addRemoteAttribute(BLERemoteAttribute& remoteAttribute) {
  if (this->_remoteAttributes == NULL) {
    this->_remoteAttributes = (BLERemoteAttribute**)malloc(BLERemoteAttribute::numAttributes() * sizeof(BLERemoteAttribute*));
  }

  this->_remoteAttributes[this->_numRemoteAttributes] = &remoteAttribute;
  this->_numRemoteAttributes++;
}

void BLEPeripheralObserver::setAdvertisingInterval(unsigned short advertisingInterval) {
  this->_device->setAdvertisingInterval(advertisingInterval);
}

void BLEPeripheralObserver::setConnectionInterval(unsigned short minimumConnectionInterval, unsigned short maximumConnectionInterval) {
  this->_device->setConnectionInterval(minimumConnectionInterval, maximumConnectionInterval);
}

void BLEPeripheralObserver::disconnect() {
  this->_device->disconnect();
}

BLECentral BLEPeripheralObserver::central() {
  this->poll();

  return this->_central;
}

bool BLEPeripheralObserver::connected() {
  this->poll();

  return this->_central;
}

void BLEPeripheralObserver::setEventHandler(BLEPeripheralObserverEvent event, BLEPeripheralObserverEventHandler eventHandler) {
  if (event < sizeof(this->_eventHandlers)) {
    this->_eventHandlers[event] = eventHandler;
  }
}

void BLEPeripheralObserver::setEventHandler(BLEDeviceEvent event, BLEDeviceEventHandler eventHandler) {
  if (event < sizeof(this->_deviceEvents)) {
    this->_deviceEvents[event] = eventHandler;
  }
}

bool BLEPeripheralObserver::characteristicValueChanged(BLECharacteristic& characteristic) {
  return this->_device->updateCharacteristicValue(characteristic);
}

bool BLEPeripheralObserver::broadcastCharacteristic(BLECharacteristic& characteristic) {
  return this->_device->broadcastCharacteristic(characteristic);
}

bool BLEPeripheralObserver::canNotifyCharacteristic(BLECharacteristic& characteristic) {
  return this->_device->canNotifyCharacteristic(characteristic);
}

bool BLEPeripheralObserver::canIndicateCharacteristic(BLECharacteristic& characteristic) {
  return this->_device->canIndicateCharacteristic(characteristic);
}

bool BLEPeripheralObserver::canReadRemoteCharacteristic(BLERemoteCharacteristic& characteristic) {
  return this->_device->canReadRemoteCharacteristic(characteristic);
}

bool BLEPeripheralObserver::readRemoteCharacteristic(BLERemoteCharacteristic& characteristic) {
  return this->_device->readRemoteCharacteristic(characteristic);
}

bool BLEPeripheralObserver::canWriteRemoteCharacteristic(BLERemoteCharacteristic& characteristic) {
  return this->_device->canWriteRemoteCharacteristic(characteristic);
}

bool BLEPeripheralObserver::writeRemoteCharacteristic(BLERemoteCharacteristic& characteristic, const unsigned char value[], unsigned char length) {
  return this->_device->writeRemoteCharacteristic(characteristic, value, length);
}

bool BLEPeripheralObserver::canSubscribeRemoteCharacteristic(BLERemoteCharacteristic& characteristic) {
  return this->_device->canSubscribeRemoteCharacteristic(characteristic);
}

bool BLEPeripheralObserver::subscribeRemoteCharacteristic(BLERemoteCharacteristic& characteristic) {
  return this->_device->subscribeRemoteCharacteristic(characteristic);
}

bool BLEPeripheralObserver::canUnsubscribeRemoteCharacteristic(BLERemoteCharacteristic& characteristic) {
  return this->_device->canUnsubscribeRemoteCharacteristic(characteristic);
}

bool BLEPeripheralObserver::unsubcribeRemoteCharacteristic(BLERemoteCharacteristic& characteristic) {
  return this->_device->unsubcribeRemoteCharacteristic(characteristic);
}

void BLEPeripheralObserver::BLEDeviceConnected(BLEDevice& /*device*/, const unsigned char* address) {
  this->_central.setAddress(address);

#ifdef BLE_PERIPHERAL_DEBUG
  Serial.print(F("Peripheral connected to central: "));
  Serial.println(this->_central.address());
#endif

  BLEPeripheralObserverEventHandler eventHandler = this->_eventHandlers[BLEConnected];
  if (eventHandler) {
    eventHandler(this->_central);
  }
}

void BLEPeripheralObserver::BLEDeviceDisconnected(BLEDevice& /*device*/) {
#ifdef BLE_PERIPHERAL_DEBUG
  Serial.print(F("Peripheral disconnected from central: "));
  Serial.println(this->_central.address());
#endif

  BLEPeripheralObserverEventHandler eventHandler = this->_eventHandlers[BLEDisconnected];
  if (eventHandler) {
    eventHandler(this->_central);
  }

  this->_central.clearAddress();
}

void BLEPeripheralObserver::BLEDeviceBonded(BLEDevice& /*device*/) {
#ifdef BLE_PERIPHERAL_DEBUG
  Serial.print(F("Peripheral bonded: "));
  Serial.println(this->_central.address());
#endif

  BLEPeripheralObserverEventHandler eventHandler = this->_eventHandlers[BLEBonded];
  if (eventHandler) {
    eventHandler(this->_central);
  }
}

void BLEPeripheralObserver::BLEDeviceRemoteServicesDiscovered(BLEDevice& /*device*/) {
#ifdef BLE_PERIPHERAL_DEBUG
  Serial.print(F("Peripheral discovered central remote services: "));
  Serial.println(this->_central.address());
#endif

  BLEPeripheralObserverEventHandler eventHandler = this->_eventHandlers[BLERemoteServicesDiscovered];
  if (eventHandler) {
    eventHandler(this->_central);
  }
}

void BLEPeripheralObserver::BLEDeviceCharacteristicValueChanged(BLEDevice& /*device*/, BLECharacteristic& characteristic, const unsigned char* value, unsigned char valueLength) {
  characteristic.setValue(this->_central, value, valueLength);
}

void BLEPeripheralObserver::BLEDeviceCharacteristicSubscribedChanged(BLEDevice& /*device*/, BLECharacteristic& characteristic, bool subscribed) {
  characteristic.setSubscribed(this->_central, subscribed);
}

void BLEPeripheralObserver::BLEDeviceRemoteCharacteristicValueChanged(BLEDevice& /*device*/, BLERemoteCharacteristic& remoteCharacteristic, const unsigned char* value, unsigned char valueLength) {
  remoteCharacteristic.setValue(this->_central, value, valueLength);
}

void BLEPeripheralObserver::BLEDeviceAddressReceived(BLEDevice& device, const unsigned char* address) {
#ifdef BLE_PERIPHERAL_DEBUG
  char addressStr[18];

  BLEUtil::addressToString(address, addressStr);

  Serial.print(F("Peripheral address: "));
  Serial.println(addressStr);
#endif
  BLEDeviceEventHandler eventHandler = this->_deviceEvents[BLEAddressReceived];
  if (eventHandler) {
    eventHandler(address);
  }
}

void BLEPeripheralObserver::BLEDeviceTemperatureReceived(BLEDevice& device, float temperature) {
  BLEDeviceEventHandler eventHandler = this->_deviceEvents[BLETemperatureReceived];
  if (eventHandler) {
    eventHandler(&temperature);
  }
}

void BLEPeripheralObserver::BLEDeviceBatteryLevelReceived(BLEDevice& device, float batteryLevel) {
  BLEDeviceEventHandler eventHandler = this->_deviceEvents[BLEBatteryLevelReceived];
  if (eventHandler) {
    eventHandler(&batteryLevel);
  }
}

void BLEPeripheralObserver::BLEDeviceAdvertisementReceived(BLEDevice& device, const unsigned char* advertisement) {
  BLEDeviceEventHandler eventHandler = this->_deviceEvents[BLEAdvertisementReceived];
  if (eventHandler) {
    eventHandler(advertisement);
  }
}

void BLEPeripheralObserver::initLocalAttributes() {
  this->_localAttributes = (BLELocalAttribute**)malloc(BLELocalAttribute::numAttributes() * sizeof(BLELocalAttribute*));

  this->_localAttributes[0] = &this->_genericAccessService;
  this->_localAttributes[1] = &this->_deviceNameCharacteristic;
  this->_localAttributes[2] = &this->_appearanceCharacteristic;

  this->_localAttributes[3] = &this->_genericAttributeService;
  this->_localAttributes[4] = &this->_servicesChangedCharacteristic;

  this->_numLocalAttributes = 5;
}
