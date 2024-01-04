#include <Arduino.h>
#include <ArduinoLog.h>
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>

#include "AdvertisedDeviceCallbacks.hh"
#include "ClientCallbacks.hh"
#include "ffi.hh"
#include "utility.hh"

#define SERIAL_BAUD_RATE       115200
#define DEVICE_NAME            "u701"
#define DEVICE_MANUFACTURER    "HVA"
#define DEVICE_BATTERY         100
#define CLIENT_CONNECT_TIMEOUT 30

static NimBLEAddress testServerAddress(0x083A8D9A444A, BLE_ADDR_PUBLIC); // TEST
static NimBLEAddress realServerAddress(0xF797AC1FF8C0, BLE_ADDR_RANDOM); // REAL
static NimBLEUUID serviceUUID("1812");
static NimBLEUUID charUUID("2a4d");

BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);
SemaphoreHandle_t incommingClientSemaphore = xSemaphoreCreateBinary();
SemaphoreHandle_t outgoingClientSemaphore  = xSemaphoreCreateBinary();
AdvertisedDeviceCallbacks advertisedDeviceCallbacks;
ClientCallbacks clientCallbacks;

bool subscribeToCharacteristic(NimBLEClient *pClient, NimBLERemoteCharacteristic *chr) {
  if (!chr->getRemoteService()->getUUID().equals(serviceUUID)) {
    Log.traceln("Service UUID does not match, skipping");
    return false;
  } else if (!chr->getUUID().equals(charUUID)) {
    Log.traceln("Characteristic UUID does not match, skipping");
    return false;
  } else if (chr->canNotify() && chr->subscribe(true, onEvent)) {
    Log.noticeln("Successfully subscribed to characteristic (notify)");
    return true;
  } else if (chr->canIndicate() && chr->subscribe(false, onEvent)) {
    Log.noticeln("Successfully subscribed to characteristic (indicate))");
    return true;
  } else {
    Log.warningln("Characteristic cannot notify or indicate, skipping");
    return false;
  }
}

extern "C" void init_arduino() {
  initArduino();

  Serial.begin(SERIAL_BAUD_RATE);
  Log.begin(LOG_LEVEL_INFO, &Serial, true);
  Log.infoln("Starting ESP32 Proxy");

  updateWatchdogTimeout(120);

  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  NimBLEDevice::init(DEVICE_NAME);

  Log.infoln("Broadcasting BLE keyboard");
  keyboard.whenClientConnects(onClientConnect);
  keyboard.whenClientDisconnects(onClientDisconnect);
  keyboard.begin();

  Log.traceln("Wait for the keyboard to connect (output) (semaphore)");
  xSemaphoreTake(outgoingClientSemaphore, portMAX_DELAY);

  NimBLEDevice::whiteListAdd(testServerAddress);
  NimBLEDevice::whiteListAdd(realServerAddress);

  updateWatchdogTimeout(5 * 60);

  Log.traceln("Starting BLE scan for the Terrain Command");

  auto pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(&advertisedDeviceCallbacks);
  pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
  pScan->setActiveScan(true);
  pScan->setMaxResults(1);

  auto results = pScan->start(0);
  auto device  = results.getDevice(0);
  auto addr    = device.getAddress();
  auto pClient = NimBLEDevice::createClient(addr);

  pClient->setClientCallbacks(&clientCallbacks, false);
  pClient->setConnectTimeout(CLIENT_CONNECT_TIMEOUT);
  pClient->setConnectionParams(12, 12, 0, 51);

  updateWatchdogTimeout(60);
  if (!pClient->connect()) {
    restart("Could not connect to the Terrain Command");
  }

  Log.noticeln("Wait for the Terrain Command to authenticate (input) (semaphore)");
  xSemaphoreTake(incommingClientSemaphore, portMAX_DELAY);

  updateWatchdogTimeout(20);
  Log.noticeln("Fetching services & characteristics");

  for (auto pService: *pClient->getServices(true)) {
    for (auto pChar: *pService->getCharacteristics(true)) {
      if (subscribeToCharacteristic(pClient, pChar)) {
        return removeWatchdog();
      }
    }
  }

  disconnect(pClient, "Failed to subscribe to characteristic");
}
