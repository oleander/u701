
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

extern "C" void ble_keyboard_write(uint8_t c[2]) {
  keyboard.write(c);
}

extern "C" void ble_keyboard_print(const uint8_t *format) {
  keyboard.print(reinterpret_cast<const char *>(format));
}

extern "C" bool ble_keyboard_is_connected() {
  return keyboard.isConnected();
}

/* Add function isActive to the State struct */
static void onEvent(BLERemoteCharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify) {
  if (length != 4) {
    Log.traceln("Received length should be 4, got %d (will continue anyway)", length);
  }

  if (!isNotify) {
    Log.traceln("Received invalid isNotify: %d (expected true)", isNotify);
    return;
  }

  Log.traceln("[Click] Received length: %d", length);
  Log.traceln("[Click] Received isNotify: %d", isNotify);

  transition_from_cpp(data, length);
}

void setupKeyboard() {
  Log.noticeln("Enable Keyboard");
  keyboard.begin();
  keyboard.setDelay(13);
}

void setupSerial() {
  // Serial.begin(SERIAL_BAUD_RATE);
  // #ifdef RELEASE
  // Log.begin(LOG_LEVEL_SILENT, &Serial);
  // #else
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  // #endif

  Log.noticeln("Starting ESP32 ...");
}

/**
 * Sets up the client to connect to the BLE device with the specified MAC address.
 * If the connection fails or no services/characteristics are found, the device will restart.
 */
void setupClient() {
  Log.noticeln("Connecting to");
  if (!client) {
    restart("[BUG] Device not found, will reboot");
  }

  Log.noticeln("Setting client callbacks ...");
  client->setClientCallbacks(new ClientCallback());

  Log.noticeln("[Connecting]");
  if (!client->connect()) {
    restart("Timeout connecting to the device");
  }

  Log.noticeln("Discovering services ...");
  auto services = client->getServices(true);
  if (services->empty()) {
    restart("[BUG] No services found, will retry");
  }

  for (auto &service: *services) {
    if (!service->getUUID().equals(hidService)) continue;

    Log.noticeln("Discovering characteristics ...");
    auto characteristics = service->getCharacteristics(true);
    if (characteristics->empty()) {
      restart("[BUG] No characteristics found");
    }

    for (auto &characteristic: *characteristics) {
      if (!characteristic->getUUID().equals(reportUUID)) continue;

      if (!characteristic->canNotify()) {
        restart("[BUG] Characteristic cannot notify");
      }

      auto status = characteristic->subscribe(true, onEvent, true);
      if (!status) {
        restart("[BUG] Failed to subscribe to notifications");
      }

      Log.noticeln("Subscribed to notifications");
      return;
    }
  }

  restart("[BUG] No report characteristic found");
}

class Callbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertised) {
    auto deviceMacAddress  = advertised->getAddress();
    auto deviceMacAsString = deviceMacAddress.toString().c_str();
    auto deviceName        = advertised->getName().c_str();

    if (deviceMacAddress != buttonMacAddress) {
      return Log.noticeln("[WRONG] %s", deviceMacAsString);
    }

    Log.noticeln("[CORRECT] %s @ %s", deviceMacAsString, deviceName);

    client = NimBLEDevice::createClient(deviceMacAddress);
    advertised->getScan()->stop();
  }
};

/**
 * Sets up the BLE scan to search for the device with the specified MAC address.
 * If the device is found, the scan will stop and the client will be set up.
 * The scan interval is set high to save power
 */
void setupScan() {
  Log.noticeln("Starting BLE scan ...");

  auto scan = NimBLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new Callbacks());
  scan->setInterval(SCAN_INTERVAL);
  scan->setWindow(SCAN_WINDOW);
  scan->setActiveScan(true);
  scan->start(0, false);

  Log.noticeln("Scan finished");
}

extern "C" void app_main() {
  NimBLEDevice::init(DEVICE_NAME);
  setupSerial();
  setupKeyboard();
  setup_rust();

  setupScan();
  Log.noticeln("Setup done");
  setupClient();
  Log.noticeln("Setup done");

  WiFi.config(ip, gateway, subnet);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  WiFi.softAP(ESP_WIFI_SSID, ESP_WIFI_PASSWORD, 1, true);

  ArduinoOTA.setPassword(ESP_OTA_PASSWORD);
  ArduinoOTA.begin();

  while (true) {
    ArduinoOTA.handle();
  }
}
