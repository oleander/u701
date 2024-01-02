// #include <BleKeyboard.h>
#include "ffi.h"
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
// NimBLEAddress ServerAddress(0xA842E3CD0C6, BLE_ADDR_RANDOM); // TEST
NimBLEAddress serverAddress(0xF797AC1FF8C0, BLE_ADDR_RANDOM); // REAL
static NimBLEUUID serviceUUID("1812");
static NimBLEUUID charUUID("2a4d");

void restart(const char *format, ...) {
  // Simplify memory management with std::vector
  std::vector<char> buffer(256);

  va_list args;
  va_start(args, format);
  int needed = vsnprintf(buffer.data(), buffer.size(), format, args);
  va_end(args);

  // Resize buffer if needed and reformat message
  if (needed >= buffer.size()) {
    buffer.resize(needed + 1);
    va_start(args, format);
    vsnprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);
  }

  // Print message and restart
  Log.fatal(buffer.data());
  Log.fatal("Will restart the ESP in 2 seconds");
  delay(2000);
  ESP.restart();
}

SemaphoreHandle_t incommingClientSemaphore = xSemaphoreCreateBinary();
SemaphoreHandle_t outgoingClientSemaphore  = xSemaphoreCreateBinary();
BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);
static NimBLEClient *pClient;

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
    pClient->updateConnParams(120, 120, 0, 1);
  };

  void onDisconnect(NimBLEClient *pClient) {
    restart("Terrain Command disconnected");
  };

  bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params) {
    Log.notice("Connection parameters update request received");
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
    Log.notice("Connection with Terrain Command established");

    if (!desc->sec_state.encrypted) {
      restart("Encrypt connection failed: %s", desc);
    }

    Log.trace("Release Terrain Command semaphore (input) (semaphore)");
    xSemaphoreGive(incommingClientSemaphore);
  };
};

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    Serial.print(".");

    esp_task_wdt_reset();

    if (!advertisedDevice->isAdvertisingService(serviceUUID)) {
      return;
    } else if (advertisedDevice->getName() != CLIENT_NAME) {
      return;
    } else if (advertisedDevice->getAddress() != serverAddress) {
      return;
    } else {
      Log.notice("\nFound the Terrain Command");
    }

    auto addr = advertisedDevice->getAddress();
    pClient   = NimBLEDevice::createClient(addr);
    advertisedDevice->getScan()->stop();
  };
};

AdvertisedDeviceCallbacks advertisedDeviceCallbacks;
ClientCallbacks clientCallbacks;

extern "C" void init_arduino() {
  esp_task_wdt_add(NULL);

  initArduino();

  Serial.begin(SERIAL_BAUD_RATE);
  Log.begin(CORE_DEBUG_LEVEL, &Serial);
  Serial.println("(0) Starting ESP32 BLE Proxy");
  Log.notice("(1) Starting ESP32 BLE Proxy");

  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  NimBLEDevice::setPower(ESP_PWR_LVL_N0);
  NimBLEDevice::init(DEVICE_NAME);

  // Setup HID keyboard and wait for the client to connect
  keyboard.whenClientConnects([](ble_gap_conn_desc *_desc) {
    Log.notice("Client connected to the keyboard");
    Log.trace("Release keyboard semaphore (output) (semaphore)");
    xSemaphoreGive(outgoingClientSemaphore);
  });

  Log.notice("Broadcasting BLE keyboard");
  keyboard.begin();

  Log.notice("Wait for the keyboard to connect (output) (semaphore)");
  xSemaphoreTake(outgoingClientSemaphore, portMAX_DELAY);

  Log.notice("Starting BLE scan for the Terrain Command");
  auto pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(&advertisedDeviceCallbacks);
  pScan->setInterval(SCAN_INTERVAL);
  pScan->setWindow(SCAN_WINDOW);
  pScan->setActiveScan(true);
  pScan->setMaxResults(0);
  pScan->start(SCAN_DURATION, false);

  if (!pClient) {
    restart("The Terrain Command was not found");
  }

  pClient->setClientCallbacks(&clientCallbacks, false);
  pClient->setConnectionParams(12, 12, 0, 51);
  pClient->setConnectTimeout(10);

  Log.notice("Wait for the Terrain Command to establish connection (input)");
  if (pClient->isConnected()) {
    Log.warning("Terrain Command already connected, will continue");
  } else if (pClient->connect()) {
    Log.notice("Successfully connected to the Terrain Command");
  } else {
    restart("Could not connect to the Terrain Command");
  }

  Log.notice("Wait for the Terrain Command to authenticate (input) (semaphore)");
  xSemaphoreTake(incommingClientSemaphore, portMAX_DELAY);

  Log.trace("Fetching service from the Terrain Command ...");
  auto pSvc = pClient->getService(serviceUUID);
  if (!pSvc) {
    Log.fatal("Failed to find our service UUID");
    Log.fatal("Will disconnect the device");
    pClient->disconnect();
    restart("Device has been manually disconnected");
  }

  Log.notice("Fetching all characteristics from the Terrain Command ...");
  auto pChrs = pSvc->getCharacteristics(true);
  if (!pChrs) {
    Log.fatal("Failed to find our characteristic UUID");
    Log.fatal("Will disconnect the device");
    pClient->disconnect();
    restart("Device has been manually disconnected");
  }

  for (auto &chr: *pChrs) {
    if (!chr->canNotify()) {
      Log.trace("Characteristic cannot notify, skipping");
      continue;
    }

    if (!chr->getUUID().equals(charUUID)) {
      Log.trace("Characteristic UUID does not match, skipping");
      continue;
    }

    if (!chr->subscribe(true, onEvent, false)) {
      Log.trace("Failed to subscribe to characteristic");
      pClient->disconnect();
      restart("Device has been manually disconnected");
    }

    Log.notice("Successfully subscribed to characteristic");
    break;
  }

  Log.notice("Setup complete");
  esp_task_wdt_deinit();
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
