
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

IPAddress ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

static NimBLEUUID reportUUID("2A4D");
static NimBLEUUID cccdUUID("2902");
static NimBLEUUID hidService("1812");
static NimBLEUUID batteryServiceUUID("180F");
static NimBLEUUID batteryLevelCharUUID("2A19");

static NimBLEClient *client;

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
    return Log.traceln("[Battery] Received length should be 1, got %d", length);
  }

  if (!isNotify) {
    return Log.traceln("[Battery] Received invalid isNotify: %d (expected true)", isNotify);
  }

  Log.traceln("[Battery] Received length: %d", length);
  Log.traceln("[Battery] Received isNotify: %d", isNotify);
  Log.traceln("[Battery] Received battery level: %d", data[0]);

  if (!keyboard.isConnected()) {
    return restart("iPhone has disconnected, will reboot");
  }

  keyboard.setBatteryLevel(data[0]);
}

void initializeKeyboard() {
  Log.noticeln("Enable Keyboard");

  keyboard.setBatteryLevel(100);
  keyboard.setDelay(12);
  keyboard.begin();

  Log.noticeln("Waiting for keyboard to connect ...");

  while (!keyboard.isConnected()) {
    Serial.print(".");
    delay(100);
  }

  auto waitingTimeinSeconds = 2;
  Log.noticeln("iPhone connected, but will wait %d seconds to be sure", waitingTimeinSeconds);
  delay(waitingTimeinSeconds * 1000);
}

void initializeSerialCommunication() {
  Serial.begin(SERIAL_BAUD_RATE);
  // Log.begin(LOG_LEVEL_SILENT, &Serial);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
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
  if (client->isConnected()) {
    return Log.noticeln("Already connected to device");
  }

  if (!client->connect()) {
    restart("Could not connect to the Terrain Command");
  }

  Log.noticeln("Discovering services ...");
  for (auto &service: *client->getServices(true)) {
    Log.noticeln("Discovering characteristics ...");
    for (auto &characteristic: *service->getCharacteristics(true)) {
      auto currentServiceUUID = service->getUUID().toString().c_str();
      auto currentCharUUID    = characteristic->getUUID().toString().c_str();

      // Register for battery level updates
      if (!service->getUUID().equals(batteryServiceUUID)) {
        Log.warningln("[Battery] Unknown battery service: %s", currentServiceUUID);
      } else if (!characteristic->getUUID().equals(batteryLevelCharUUID)) {
        Log.warningln("[Battery] Unknown battery characteristic: %s", currentCharUUID);
      } else if (!characteristic->canNotify()) {
        Log.warningln("[Battery] Cannot subscribe to notifications: %s", currentCharUUID);
      } else if (!characteristic->subscribe(true, handleBatteryUpdate, true)) {
        Log.errorln("[BUG] [Battery] Failed to subscribe to notifications: %s", currentCharUUID);
      } else {
        Log.noticeln("[Battery] Subscribed to notifications: %s", currentCharUUID);
      }

      // Register for click events
      if (!service->getUUID().equals(hidService)) {
        Log.warningln("[Click] Unknown report service: %s", currentServiceUUID);
      } else if (!characteristic->getUUID().equals(reportUUID)) {
        Log.warningln("[Click] Unknown report characteristic: %s", currentCharUUID);
      } else if (!characteristic->canNotify()) {
        Log.warningln("[Click] Cannot subscribe to notifications: %s", currentCharUUID);
      } else if (!characteristic->subscribe(true, handleButtonClick, true)) {
        Log.errorln("[Click] [Bug] Failed to subscribe to notifications: %s", currentCharUUID);
      } else {
        Log.noticeln("[Click] Subscribed to notifications: %s", currentCharUUID);
      }
    }
  }

  Log.noticeln("Subcribed to all characteristics");
}

class Callbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertised) {
    auto macAddr = advertised->getAddress();

    if (macAddr != buttonMacAddress) {
      Serial.print(".");
      return;
    }

    client = NimBLEDevice::createClient(macAddr);
    advertised->getScan()->stop();

    Log.noticeln("[SCAN] Terrain Command found");
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
}

extern "C" void configure_ota() {
  Log.noticeln("Configuring WiFi ...");

  WiFi.config(ip, gateway, subnet);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  WiFi.softAP(ESP_WIFI_SSID, ESP_WIFI_PASSWORD, 1, true);

  ArduinoOTA.setPassword(ESP_OTA_PASSWORD);

  ArduinoOTA.onEnd([]() {
    Log.noticeln("OTA update finished, will reboot");
    restart("OTA update finished, will reboot");
  });

  ArduinoOTA.begin();
}

void setup() {
  initializeSerialCommunication();
  initializeKeyboard();
  setup_rust();
  startBLEScanForDevice();
  connectToClientDevice();
}

void loop() {
  process_ble_events();
}
