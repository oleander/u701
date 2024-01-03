// #include <BleKeyboard.h>
#include "ffi.h"
#include "utility.h"
#include <Arduino.h>
#include <ArduinoLog.h>
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
#include <esp_task_wdt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <vector>

#define SCAN_DURATION 5 * 60 // in seconds
#define SCAN_INTERVAL 500    // in ms
#define SCAN_WINDOW   450    // in ms

#define SERIAL_BAUD_RATE 115200
#define DEVICE_BATTERY   100

#define CLIENT_NAME         "Terrain Comman"
#define DEVICE_NAME         "u701"
#define DEVICE_MANUFACTURER "HVA"

// A8:42:E3:CD:FB:C6, f7:97:ac:1f:f8:c0
// 08:3a:8d:9a:44:4a
NimBLEAddress testServerAddress(0x083A8D9A444A); // TEST
NimBLEAddress realServerAddress(0xF797AC1FF8C0); // REAL
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

// Terrain Command BLE buttons
class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *pClient) {
    Log.notice("Connected to Terrain Command");
    Log.trace("Update connection parameters");
    pClient->updateConnParams(120, 120, 0, 60);
  };

  void onDisconnect(NimBLEClient *pClient) {
    restart("Disconnected from Terrain Command");
  };

  bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params) {
    Log.trace("Requested connection params: interval: %d, latency: %d, supervision timeout: %d\n",
              params->itvl_min,
              params->latency,
              params->supervision_timeout);

    if (params->itvl_min < 24) { /** 1.25ms units */
      return false;
    } else if (params->itvl_max > 40) { /** 1.25ms units */
      return false;
    } else if (params->latency > 2) { /** Number of intervals allowed to skip */
      return false;
    } else if (params->supervision_timeout > 100) { /** 10ms units */
      return false;
    }

    return true;
  };

  /** Pairing proces\s complete, we can check the results in ble_gap_conn_desc */
  void onAuthenticationComplete(ble_gap_conn_desc *desc) {
    if (!desc->sec_state.encrypted) {
      restart("Encrypt connection failed: %s", desc);
    }

    Log.notice("Secure connection to Terrain Command established");
    Log.trace("Release Terrain Command semaphore (input) (semaphore)");
    xSemaphoreGive(incommingClientSemaphore);
  };
};

bool isValidAdvertisement(NimBLEAdvertisedDevice *advertisedDevice) {
  Log.trace("Found a new device");
  if (!advertisedDevice->haveServiceUUID()) {
    Log.trace("\twith no service UUID");
    return false;
  }

  if (!advertisedDevice->isAdvertisingService(serviceUUID)) {
    Log.trace("Does not advertise %s, got %s",
              serviceUUID.toString().c_str(),
              advertisedDevice->getServiceUUID().toString().c_str());
    return false;
  }

  // Check if name is the Terrain Comman or key
  auto e1   = "Terrain Comman";
  auto e2   = "key";
  auto name = advertisedDevice->getName();
  if (name != e1 && name != e2) {
    Log.trace("Does not advertise %s or %s, got %s", e1, e2, name.c_str());
    return false;
  }

  auto serverAddress = advertisedDevice->getAddress();
  if (serverAddress != testServerAddress && serverAddress != realServerAddress) {
    Log.trace("Does not advertise %s or %s, got %s",
              testServerAddress.toString().c_str(),
              realServerAddress.toString().c_str(),
              serverAddress.toString().c_str());
    return false;
  }

  Log.trace("Found BLE client named %s", name.c_str());
  return true;
}
/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    if (isValidAdvertisement(advertisedDevice)) {
      advertisedDevice->getScan()->stop();
    } else {
      advertisedDevice->getScan()->clearResults();
    }
  };
};

AdvertisedDeviceCallbacks advertisedDeviceCallbacks;
ClientCallbacks clientCallbacks;

void connectToClient(void *client) {
  auto pClient = static_cast<NimBLEClient *>(client);
}

extern "C" void init_arduino() {
  initArduino();

  Serial.begin(SERIAL_BAUD_RATE);
  // Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.notice("Starting ESP32 Proxy");

  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  NimBLEDevice::setPower(ESP_PWR_LVL_N0);
  NimBLEDevice::init(DEVICE_NAME);

  // Setup HID keyboard and wait for the client to connect
  keyboard.whenClientConnects([](ble_gap_conn_desc *_desc) {
    Serial.println("Client connected to the keyboard");
    Serial.println("Release keyboard semaphore (output) (semaphore)");
    xSemaphoreGive(outgoingClientSemaphore);
  });

  // Restart the ESP if the client disconnects
  keyboard.whenClientDisconnects(
      [](BLEServer *_server) { restart("Client disconnected from the keyboard, will restart"); });

  Serial.println("Broadcasting BLE keyboard");
  keyboard.begin();

  Serial.println("Wait for the keyboard to connect (output) (semaphore)");
  xSemaphoreTake(outgoingClientSemaphore, portMAX_DELAY);

  NimBLEDevice::whiteListAdd(testServerAddress);
  NimBLEDevice::whiteListAdd(realServerAddress);

  Serial.println("Starting BLE scan for the Terrain Command");

  auto pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(&advertisedDeviceCallbacks);
  pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
  pScan->setInterval(SCAN_INTERVAL);
  pScan->setWindow(SCAN_WINDOW);
  pScan->setLimitedOnly(false);
  pScan->setActiveScan(true);
  pScan->setMaxResults(1);
  pScan->start(SCAN_DURATION, false);

  auto advDevice = pScan->getResults().getDevice(0);
  auto addr      = advDevice.getAddress();
  auto pClient   = NimBLEDevice::createClient(addr);

  pClient->setClientCallbacks(&clientCallbacks, false);
  pClient->setConnectionParams(12, 12, 0, 51);
  pClient->setConnectTimeout(10);

  Serial.println("Wait for the Terrain Command to establish connection (input)");
  if (pClient->isConnected()) {
    Serial.println("Terrain Command already connected, will continue");
  } else if (pClient->connect()) {
    Serial.println("Successfully connected to the Terrain Command");
  } else {
    restart("Could not connect to the Terrain Command");
  }

  Serial.println("Wait for the Terrain Command to authenticate (input) (semaphore)");
  xSemaphoreTake(incommingClientSemaphore, 10000 / portTICK_PERIOD_MS);

  Serial.println("Fetching service from the Terrain Command ...");
  auto pSvc = pClient->getService(serviceUUID);
  if (!pSvc) {
    Serial.println("[BUG] Failed to find our service UUID");
    Serial.println("Will disconnect the device");
    pClient->disconnect();
    restart("Device has been manually disconnected");
  }

  Serial.println("Fetching all characteristics from the Terrain Command ...");
  auto pChrs = pSvc->getCharacteristics(true);
  if (!pChrs) {
    Serial.println("[BUG] Failed to find our characteristic UUID");
    Serial.println("Will disconnect the device");
    pClient->disconnect();
    restart("Device has been manually disconnected");
  }

  for (auto &chr: *pChrs) {
    if (!chr->canNotify()) {
      Serial.println("Characteristic cannot notify, skipping");
      continue;
    }

    if (!chr->getUUID().equals(charUUID)) {
      Serial.println("Characteristic UUID does not match, skipping");
      continue;
    }

    if (!chr->subscribe(true, onEvent, false)) {
      Serial.println("[BUG] Failed to subscribe to characteristic");
      pClient->disconnect();
      restart("Device has been manually disconnected");
    }

    Serial.println("Successfully subscribed to characteristic");
    return;
  }

  restart("Failed to find our characteristic UUID");
}

extern "C" void ble_keyboard_write(uint8_t c[2]) {
  if (keyboard.isConnected()) {
    keyboard.write(c);
  }
}

extern "C" void ble_keyboard_print(const uint8_t *format) {
  if (keyboard.isConnected()) {
    keyboard.print(reinterpret_cast<const char *>(format));
  }
}

extern "C" bool ble_keyboard_is_connected() {
  return keyboard.isConnected();
}
