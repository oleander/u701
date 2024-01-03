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

/* Event received from the Terrain Command */
static void onEvent(BLERemoteCharacteristic *_, uint8_t *data, size_t length, bool isNotify) {
  if (!isNotify) {
    return;
  }

  c_on_event(data, length);
}

void printPrefix(Print *_logOutput, int logLevel) {
  _logOutput->printf("(%02d) ", millis());
}

// Terrain Command BLE buttons
class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *pClient) {
    Log.noticeln("Connected to Terrain Command");
    Log.traceln("Update connection parameters");
    pClient->updateConnParams(120, 120, 0, 60);
  };

  void onDisconnect(NimBLEClient *pClient) {
    restart("Disconnected from Terrain Command");
  };

  // bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params) {
  //   Log.traceln("Requested connection params: interval: %d, latency: %d, supervision timeout: %d\n",
  //               params->itvl_min,
  //               params->latency,
  //               params->supervision_timeout);

  //   if (params->itvl_min < 24) { /** 1.25ms units */
  //     return false;
  //   } else if (params->itvl_max > 40) { /** 1.25ms units */
  //     return false;
  //   } else if (params->latency > 2) { /** Number of intervals allowed to skip */
  //     return false;
  //   } else if (params->supervision_timeout > 100) { /** 10ms units */
  //     return false;
  //   }

  //   return true;
  // };

  /** Pairing proces\s complete, we can check the results in ble_gap_conn_desc */
  void onAuthenticationComplete(ble_gap_conn_desc *desc) {
    if (!desc->sec_state.encrypted) {
      restart("Encrypt connection failed: %s", desc);
    }

    Log.noticeln("Secure connection to Terrain Command established");
    Log.traceln("Release Terrain Command semaphore (input) (semaphore)");
    xSemaphoreGive(incommingClientSemaphore);
  };
};

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    advertisedDevice->getScan()->stop();
  };
};

AdvertisedDeviceCallbacks advertisedDeviceCallbacks;
ClientCallbacks clientCallbacks;

extern "C" void init_arduino() {
  initArduino();

  Serial.begin(SERIAL_BAUD_RATE);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);

  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  NimBLEDevice::init(DEVICE_NAME);

  updateWatchdogTimeout(120);

  Log.infoln("Starting ESP32 Proxy");

  // Setup HID keyboard and wait for the client to connect
  keyboard.whenClientConnects([](ble_gap_conn_desc *_desc) {
    Log.traceln("Connected to keyboard");
    Log.traceln("Release keyboard semaphore (output) (semaphore)");
    xSemaphoreGive(outgoingClientSemaphore);
  });

  // Restart the ESP if the client disconnects
  keyboard.whenClientDisconnects(
      [](BLEServer *_server) { restart("Client disconnected from the keyboard, will restart"); });

  Log.infoln("Broadcasting BLE keyboard");
  keyboard.begin();

  Log.traceln("Wait for the keyboard to connect (output) (semaphore)");
  xSemaphoreTake(outgoingClientSemaphore, portMAX_DELAY);

  NimBLEDevice::whiteListAdd(testServerAddress);
  NimBLEDevice::whiteListAdd(realServerAddress);

  updateWatchdogTimeout(5 * 60);

  Log.infoln("Starting BLE scan for the Terrain Command");
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

  Log.noticeln("Wait for the Terrain Command to establish connection (input)");
  if (pClient->isConnected()) {
    Log.warning("Terrain Command already connected, will continue");
  } else if (pClient->connect()) {
    Log.infoln("Successfully connected to the Terrain Command");
  } else {
    restart("Could not connect to the Terrain Command");
  }

  Log.noticeln("Wait for the Terrain Command to authenticate (input) (semaphore)");
  xSemaphoreTake(incommingClientSemaphore, portMAX_DELAY);

  Log.noticeln("Fetching service from the Terrain Command ...");
  auto pSvc = pClient->getService(serviceUUID);
  if (!pSvc) {
    Log.fatalln("[BUG] Failed to find our service UUID");
    Log.fatalln("Will disconnect the device");
    pClient->disconnect();
    restart("Device has been manually disconnected");
  }

  Log.noticeln("Fetching all characteristics from the Terrain Command ...");
  auto pChrs = pSvc->getCharacteristics(true);
  if (!pChrs) {
    Log.fatalln("[BUG] Failed to find our characteristic UUID");
    Log.fatalln("Will disconnect the device");
    pClient->disconnect();
    restart("Device has been manually disconnected");
  }

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
      Log.fatalln("[BUG] Failed to subscribe to characteristic");
      pClient->disconnect();
      restart("Device has been manually disconnected");
    }

    Log.infoln("Successfully subscribed to characteristic");
    removeWatchdog();
    return;
  }

  restart("Failed to find our characteristic UUID");
}
