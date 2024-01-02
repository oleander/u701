// #include <BleKeyboard.h>
#include "ffi.h"
#include <Arduino.h>
// #include <ArduinoLog.h>
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
NimBLEAddress serverAddress(0x083A8D9A444A);
// NimBLEAddress serverAddress(0xF797AC1FF8C0, BLE_ADDR_RANDOM); // REAL
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
  Serial.println(buffer.data());
  Serial.println("Will restart the ESP in 2 seconds");
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
    Serial.println("Connected to Terrain Command");
    // Serial.println("Update connection parameters");
    // pClient->updateConnParams(120, 120, 0, 1);
  };

  void onDisconnect(NimBLEClient *pClient) {
    restart("Terrain Command disconnected");
  };

  bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params) {
    Serial.println("Connection parameters update request received");
    // Serial.println("Requested connection params: interval: %d, latency: %d, supervision timeout: %d\n",
    //           params->itvl_min,
    //           params->latency,
    //           params->supervision_timeout);

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
    Serial.println("Connection with Terrain Command established");

    if (!desc->sec_state.encrypted) {
      restart("Encrypt connection failed: %s", desc);
    }

    Serial.println("Release Terrain Command semaphore (input) (semaphore)");
    xSemaphoreGive(incommingClientSemaphore);
  };
};

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    Serial.print(".");

    if (advertisedDevice->getAddress() != serverAddress) {
      return;
    } else {
      Serial.println("\nFound the Terrain Command");
    }

    auto addr = advertisedDevice->getAddress();
    pClient   = NimBLEDevice::createClient(addr);
    advertisedDevice->getScan()->stop();
  };
};

AdvertisedDeviceCallbacks advertisedDeviceCallbacks;
ClientCallbacks clientCallbacks;

void checkKeyboardConnection(void *pvParameters) {
  while (true) {
    if (!keyboard.isConnected()) {
      restart("Keyboard is not connected");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

extern "C" void init_arduino() {
  esp_task_wdt_init(60, true);
  esp_task_wdt_add(NULL);

  initArduino();

  Serial.begin(SERIAL_BAUD_RATE);
  // Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Serial.println("Starting ESP32 BLE Proxy (1)");
  Serial.println("Starting ESP32 BLE Proxy (2)");

  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  // NimBLEDevice::setPower(ESP_PWR_LVL_N0);
  NimBLEDevice::init(DEVICE_NAME);

  // Setup HID keyboard and wait for the client to connect
  keyboard.whenClientConnects([](ble_gap_conn_desc *_desc) {
    Serial.println("Client connected to the keyboard");
    Serial.println("Release keyboard semaphore (output) (semaphore)");
    xSemaphoreGive(outgoingClientSemaphore);
  });

  Serial.println("Broadcasting BLE keyboard");
  keyboard.begin();

  Serial.println("Wait for the keyboard to connect (output) (semaphore)");
  xSemaphoreTake(outgoingClientSemaphore, portMAX_DELAY);

  Serial.println("Starting keyboard connection check task");
  xTaskCreate(checkKeyboardConnection, "keyboard", 2048, NULL, 5, NULL);

  Serial.println("Disble watchdog");
  esp_task_wdt_delete(NULL);

  Serial.println("Starting BLE scan for the Terrain Command");
  auto pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(&advertisedDeviceCallbacks);
  pScan->setInterval(SCAN_INTERVAL);
  pScan->setWindow(SCAN_WINDOW);
  pScan->setActiveScan(true);
  pScan->setMaxResults(0);
  pScan->start(SCAN_DURATION, false);

  Serial.println("Enable watch dog");
  esp_task_wdt_init(60, true);
  esp_task_wdt_add(NULL);

  if (!pClient) {
    restart("The Terrain Command was not found");
  }

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
  xSemaphoreTake(incommingClientSemaphore, portMAX_DELAY);

  Serial.println("Fetching service from the Terrain Command ...");
  auto pSvc = pClient->getService(serviceUUID);
  if (!pSvc) {
    Serial.println("Failed to find our service UUID");
    Serial.println("Will disconnect the device");
    pClient->disconnect();
    restart("Device has been manually disconnected");
  }

  Serial.println("Fetching all characteristics from the Terrain Command ...");
  auto pChrs = pSvc->getCharacteristics(true);
  if (!pChrs) {
    Serial.println("Failed to find our characteristic UUID");
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
      Serial.println("Failed to subscribe to characteristic");
      pClient->disconnect();
      restart("Device has been manually disconnected");
    }

    Serial.println("Successfully subscribed to characteristic");
    break;
  }

  Serial.println("Setup complete");

  Serial.println("Disable watch dog");
  esp_task_wdt_delete(NULL);
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
