
#include "ClientCallback.h"
#include "ffi.h"
#include "settings.h"
#include "utility.h"

#include <ArduinoLog.h>
#include <ArduinoOTA.h>
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
#include <WiFi.h>

#include "shared.h"

IPAddress ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

const auto buttonMacAddress = NimBLEAddress(DEVICE_MAC, 1);

BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);

extern "C" void process_ble_events();

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

/* Add function isActive to the State struct */
static void handleButtonClick(BLERemoteCharacteristic *_, uint8_t *data, size_t length, bool isNotify) {
  if (length != 4) {
    Log.traceln("Received length should be 4, got %d (will continue anyway)", length);
  }

  if (!isNotify) {
    Log.traceln("Received invalid isNotify: %d (expected true)", isNotify);
    return;
  }

  Log.traceln("[Click] Received length: %d", length);
  Log.traceln("[Click] Received isNotify: %d", isNotify);

  handle_external_click_event(data, length);
}

static void handleBatteryUpdate(BLERemoteCharacteristic *_, uint8_t *data, size_t length, bool isNotify) {
  if (length != 1) {
    Log.traceln("Received length should be 1, got %d", length);
    return;
  }

  if (!isNotify) {
    Log.traceln("Received invalid isNotify: %d (expected true)", isNotify);
    return;
  }

  Log.traceln("[Battery] Received length: %d", length);
  Log.traceln("[Battery] Received isNotify: %d", isNotify);
  Log.traceln("[Battery] Received battery level: %d", data[0]);

  if (keyboard.isConnected()) {
    keyboard.setBatteryLevel(data[0]);
  }
}

void initializeKeyboard() {
  Log.noticeln("Enable Keyboard");
  keyboard.begin();
}

void initializeSerialCommunication() {
  Serial.begin(SERIAL_BAUD_RATE);
  // Log.begin(LOG_LEVEL_SILENT, &Serial);
  // #ifdef RELEASE
  // #else
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  // #endif

  Log.noticeln("Starting ESP32 ...");
}

/**
 * Sets up the client to connect to the BLE device with the specified MAC address.
 * If the connection fails or no services/characteristics are found, the device will restart.
 */
void connectToClientDevice() {
  Log.noticeln("[Connecting] to Terrain Command ...");

  if (client == nullptr) {
    restart("Device not found, will reboot");
  }

  static ClientCallback clientCallbackInstance;
  client->setClientCallbacks(&clientCallbackInstance);
  if (!client->connect()) {
    restart("Timeout connecting to the device");
  }

  Log.noticeln("Discovering services ...");
  for (auto &service: *client->getServices(true)) {
    Log.noticeln("Discovering characteristics ...");
    for (auto &characteristic: *service->getCharacteristics(true)) {
      // Register for battery level updates
      if (!service->getUUID().equals(batteryServiceUUID)) {
        Log.noticeln("[Battery] Unknown battery service");
      } else if (!characteristic->getUUID().equals(batteryLevelCharUUID)) {
        Log.noticeln("[Battery] Unknown battery characteristic");
      } else if (!characteristic->canNotify()) {
        Log.noticeln("[Battery] Cannot subscribe to notifications");
      } else if (!characteristic->subscribe(true, handleBatteryUpdate, true)) {
        Log.errorln("[BUG] [Battery] Failed to subscribe to notifications");
      } else {
        Log.noticeln("[Battery] Subscribed to notifications");
      }

      // Register for click events
      if (!service->getUUID().equals(hidService)) {
        Log.warningln("[Click] Unknown report service");
      } else if (!characteristic->getUUID().equals(reportUUID)) {
        Log.warningln("[Click] Unknown report characteristic");
      } else if (!characteristic->canNotify()) {
        Log.warningln("[Click] Cannot subscribe to notifications");
      } else if (!characteristic->subscribe(true, handleButtonClick, true)) {
        Log.errorln("[Click] [Bug] Failed to subscribe to notifications");
      } else {
        Log.noticeln("[Click] Subscribed to notifications");
      }
    }
  }

  Log.noticeln("Subcribed to all characteristics");
}

class Callbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertised) {
    auto macAddr = advertised->getAddress();

    if (macAddr != buttonMacAddress) {
      return Log.noticeln("[WRONG]");
    }

    client = NimBLEDevice::createClient(macAddr);
    advertised->getScan()->stop();

    auto mac  = macAddr.toString().c_str();
    auto name = advertised->getName();
    Log.noticeln("[CORRECT] %s @ %s", mac, name);
  }
};

/**
 * Sets up the BLE scan to search for the device with the specified MAC address.
 * If the device is found, the scan will stop and the client will be set up.
 * The scan interval is set high to save power
 */
void startBLEScanForDevice() {
  Log.noticeln("Starting BLE scan ...");

  auto scan = NimBLEDevice::getScan();
  static Callbacks scanCallbackInstance;
  scan->setAdvertisedDeviceCallbacks(&scanCallbackInstance);
  scan->setInterval(SCAN_INTERVAL);
  scan->setWindow(SCAN_WINDOW);
  scan->setActiveScan(true);
  scan->start(0, false);

  Log.noticeln("Scan finished");
}

void configureWiFi() {
  Log.noticeln("Starting WiFi ...");

  WiFi.config(ip, gateway, subnet);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  WiFi.softAP(ESP_WIFI_SSID, ESP_WIFI_PASSWORD, 1, true);

  ArduinoOTA.setPassword(ESP_OTA_PASSWORD);
  ArduinoOTA.begin();
}

void setup() {
  initializeSerialCommunication();
  initializeKeyboard();
  setup_rust();
  startBLEScanForDevice();
  connectToClientDevice();
  configureWiFi();

  // if (keyboard.isConnected()) {
  //   keyboard.setBatteryLevel(50);
  // }
}

void loop() {
  ArduinoOTA.handle();
  process_ble_events();
}
