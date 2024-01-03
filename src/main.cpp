// #include <BleKeyboard.h>
#include "ffi.h"
#include "utility.h"
#include <Arduino.h>
#include <ArduinoLog.h>
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
#include <stdarg.h>
#include <stdlib.h>
#include <vector>

#include "AdvertisedDeviceCallbacks.h"
#include "ClientCallbacks.h"

#define SCAN_DURATION 5 * 60 // in seconds
#define SCAN_INTERVAL 500    // in ms
#define SCAN_WINDOW   450    // in ms

#define SERIAL_BAUD_RATE 115200
#define DEVICE_BATTERY   100

#define REAL_CLIENT_NAME    "Terrain Comman"
#define TEST_CLIENT_NAME    "key"
#define DEVICE_NAME         "u701"
#define DEVICE_MANUFACTURER "HVA"

NimBLEAddress testServerAddress(0x083A8D9A444A);                  // TEST
NimBLEAddress realServerAddress(0xF797AC1FF8C0, BLE_ADDR_RANDOM); // REAL
static NimBLEUUID serviceUUID("1812");
static NimBLEUUID charUUID("2a4d");

SemaphoreHandle_t incommingClientSemaphore = xSemaphoreCreateBinary();
SemaphoreHandle_t outgoingClientSemaphore  = xSemaphoreCreateBinary();
BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);

ClientCallbacks clientCallbacks;
AdvertisedDeviceCallbacks advertisedDeviceCallbacks;

/* Event received from the Terrain Command */
static void onEvent(BLERemoteCharacteristic *_, uint8_t *data, size_t length, bool isNotify) {
  if (!isNotify) {
    return;
  }

  c_on_event(data, length);
}

void onClientConnect(ble_gap_conn_desc *_desc) {
  Log.traceln("Connected to keyboard");
  Log.traceln("Release keyboard semaphore (output) (semaphore)");
  xSemaphoreGive(outgoingClientSemaphore);
}

void onClientDisconnect(BLEServer *_server) {
  restart("Client disconnected from the keyboard");
}

void disconnect(NimBLEClient *pClient, const char *format, ...) {
  Log.traceln("Disconnect from Terrain Command");
  pClient->disconnect();
  restart(format);
}

extern "C" void init_arduino() {
  initArduino();

  Serial.begin(SERIAL_BAUD_RATE);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
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

  pClient->setClientCallbacks(&clientCallbacks);
  pClient->setConnectionParams(12, 12, 0, 51);
  pClient->setConnectTimeout(10);

  updateWatchdogTimeout(60);

  if (!pClient->connect()) {
    restart("Could not connect to the Terrain Command");
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
      return Log.traceln("Characteristic cannot notify, skipping");
    }

    if (!chr->getUUID().equals(charUUID)) {
      return Log.traceln("Characteristic UUID does not match, skipping");
    }

    if (!chr->subscribe(true, onEvent, false)) {
      disconnect(pClient, "Failed to subscribe to characteristic");
    }

    Log.infoln("Successfully subscribed to characteristic");
    removeWatchdog();
    return;
  }

  restart("Failed to find our characteristic UUID");
}
