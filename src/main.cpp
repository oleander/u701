#include <Arduino.h>
#include <ArduinoLog.h>
// #include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>

#include "AdvertisedDeviceCallbacks.h"
#include "ClientCallbacks.h"
#include "ffi.h"
#include "utility.h"

#define SERIAL_BAUD_RATE       115200
#define DEVICE_NAME            "u701"
#define DEVICE_MANUFACTURER    "HVA"
#define DEVICE_BATTERY         100
#define CLIENT_CONNECT_TIMEOUT 30

static NimBLEAddress testServerAddress(0x083A8D9A444A, BLE_ADDR_RANDOM); // TEST
static NimBLEAddress realServerAddress(0xF797AC1FF8C0, BLE_ADDR_RANDOM); // REAL
static NimBLEUUID serviceUUID("1812");
static NimBLEUUID charUUID("2a4d");

// BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);
SemaphoreHandle_t incommingClientSemaphore = xSemaphoreCreateBinary();
SemaphoreHandle_t outgoingClientSemaphore  = xSemaphoreCreateBinary();
AdvertisedDeviceCallbacks advertisedDeviceCallbacks;
ClientCallbacks clientCallbacks;

extern "C" void init_arduino() {
  initArduino();

  Serial.begin(SERIAL_BAUD_RATE);
  Log.begin(LOG_LEVEL_NOTICE, &Serial, true);
  Log.infoln("Starting ESP32 Proxy");

  updateWatchdogTimeout(120);

  NimBLEDevice::init(DEVICE_NAME);
  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);

  // Log.infoln("Broadcasting BLE keyboard");
  // keyboard.whenClientConnects(onClientConnect);
  // keyboard.whenClientDisconnects(onClientDisconnect);
  // keyboard.begin();

  // Log.traceln("Wait for the keyboard to connect (output) (semaphore)");
  // xSemaphoreTake(outgoingClientSemaphore, portMAX_DELAY);

  // NimBLEDevice::whiteListAdd(testServerAddress);
  // NimBLEDevice::whiteListAdd(realServerAddress);

  updateWatchdogTimeout(5 * 60);

  // Log.traceln("Starting BLE scan for the Terrain Command");

  auto pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(&advertisedDeviceCallbacks);
  // // pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
  pScan->setActiveScan(true);
  pScan->setMaxResults(1);

  auto results = pScan->start(0);
  auto device  = results.getDevice(0);
  auto addr    = device.getAddress();
  auto pClient = NimBLEDevice::createClient(addr);

  pClient->setClientCallbacks(&clientCallbacks);
  pClient->setConnectTimeout(CLIENT_CONNECT_TIMEOUT);

  updateWatchdogTimeout(60);

  if (!pClient->connect()) {
    // restart("Could not connect to the Terrain Command");
    Log.errorln("Could not connect to the Terrain Command");
  }

  Log.noticeln("Wait for the Terrain Command to authenticate (input) (semaphore)");
  xSemaphoreTake(incommingClientSemaphore, portMAX_DELAY);

  Log.noticeln("Fetching service from the Terrain Command ...");
  auto pSvc = pClient->getService(serviceUUID);
  if (!pSvc) disconnect(pClient, "Failed to find our service UUID");

  Log.noticeln("Fetching all characteristics from the Terrain Command ...");
  auto pChrs = pSvc->getCharacteristics(true);
  if (!pChrs) disconnect(pClient, "Failed to find our characteristic UUID");

  for (auto &chr: *pChrs) {
    if (!chr->canNotify()) {
      Log.traceln("Characteristic cannot notify, skipping");
      continue;
    }

    if (!chr->getUUID().equals(charUUID)) {
      Log.traceln("Characteristic UUID does not match, skipping");
      continue;
    }

    if (!chr->subscribe(true, onEvent, false)) {
      disconnect(pClient, "Failed to subscribe to characteristic");
    }

    Log.infoln("Successfully subscribed to characteristic");
    removeWatchdog();
    return;
  }

  disconnect(pClient, "Failed to subscribe");
}
