
#include "ClientCallback.h"
#include "ffi.h"
#include "settings.h"
#include "utility.h"

#include <ArduinoLog.h>
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
// #include <WiFi.h>
// #include <ArduinoOTA.h>
// #include <esp_task_wdt.h>

#include "shared.h"

IPAddress ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

const auto buttonMacAddress = NimBLEAddress(DEVICE_MAC, 1);

BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);

extern "C" void sendCharacterViaBleKeyboard(uint8_t c[2]) {
  if (keyboard.isConnected()) {
    keyboard.write(c);
  }
}

extern "C" void printStringViaBleKeyboard(const char *format) {
  if (keyboard.isConnected()) {
    keyboard.print(reinterpret_cast<const char *>(format));
  }
}

extern "C" bool isBleKeyboardConnected() {
  return keyboard.isConnected();
}

class AdvertisedDeviceResultHandler : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertised) {
    auto deviceMacAddress  = advertised->getAddress();
    auto deviceMacAsString = deviceMacAddress.toString().c_str();
    auto deviceName        = advertised->getName();

    if (deviceMacAddress != buttonMacAddress) {
      return Log.noticeln("[WRONG] %s", deviceMacAsString);
    }

    Log.noticeln("[CORRECT] %s @ %s", deviceMacAsString, deviceName);

    client = NimBLEDevice::createClient(deviceMacAddress);
    advertised->getScan()->stop();
  }
};

/* Add function isActive to the State struct */
static void handleBLERemoteEvent(BLERemoteCharacteristic *_, uint8_t *data, size_t length, bool isNotify) {
  if (length != 4) {
    Log.traceln("Received length should be 4, got %d (will continue anyway)", length);
  }

  if (!isNotify) {
    Log.traceln("Received invalid isNotify: %d (expected true)", isNotify);
    return;
  }

  Log.traceln("[Click] Received length: %d", length);
  Log.traceln("[Click] Received isNotify: %d", isNotify);

  handleEventFromCpp(data, length);
}

void initializeKeyboard() {
  Log.noticeln("Enable Keyboard");
  keyboard.begin();
}

void initializeSerialCommunication() {
  // Log.begin(LOG_LEVEL_SILENT, &Serial);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.noticeln("Starting ESP32 ...");
}

/**
 * Sets up the client to connect to the BLE device with the specified MAC address.
 * If the connection fails or no services/characteristics are found, the device will restart.
 */
void initializeAndConnectClient() {
  if (!client) {
    restart("[BUG] Device not found, will reboot");
  }

  Log.noticeln("Connecting to client: %s", client->getPeerAddress());
  client->setClientCallbacks(new ClientCallback());

  Log.noticeln("\tConnecting");
  if (!client->connect()) {
    restart("\tTimeout connecting to device");
  }

  Log.noticeln("\tDiscovering services");
  auto services = client->getServices(true);
  if (services->empty()) {
    restart("\t[BUG] No services found, will retry");
  }

  Log.noticeln("\tReceived %d services", services->size());

  for (auto &service: *services) {
    if (!service->getUUID().equals(hidService)) {
      Log.warningln("\tSkipping service: %s", service->getUUID());
      continue;
    }

    Log.noticeln("\tDiscovering characteristics ...");
    auto characteristics = service->getCharacteristics(true);
    if (characteristics->empty()) {
      restart("\t[BUG] No characteristics found");
    }

    Log.noticeln("\tReceived %d characteristics", characteristics->size());
    for (auto &characteristic: *characteristics) {
      if (!characteristic->getUUID().equals(reportUUID)) {
        Log.warningln("\tSkipping characteristic: %s", characteristic->getUUID());
        continue;
      }

      Log.noticeln("\tFound report characteristic: %s", characteristic->getUUID());

      if (!characteristic->canNotify()) {
        restart("\t[BUG] Characteristic cannot notify");
      }

      Log.noticeln("\tSubscribing to characteristic ...");
      auto status = characteristic->subscribe(true, handleBLERemoteEvent, true);
      if (!status) {
        restart("\t[BUG] Failed to subscribe to notifications");
      }

      Log.noticeln("\tSubscribed to notifications");
      return;
    }
  }

  restart("[BUG] No report characteristic found");
}

/**
 * Sets up the BLE scan to search for the device with the specified MAC address.
 * If the device is found, the scan will stop and the client will be set up.
 * The scan interval is set high to save power
 */
void startBLEDeviceScan() {
  Log.noticeln("Starting BLE scan ...");

  auto scan = NimBLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceResultHandler());
  scan->setInterval(SCAN_INTERVAL);
  scan->setWindow(SCAN_WINDOW);
  scan->setActiveScan(true);
  scan->start(0, false);

  Log.noticeln("Scan finished");
}

void setup() {
  initializeSerialCommunication();
  initializeKeyboard();
  setupRust();
  startBLEDeviceScan();
  initializeAndConnectClient();
}

void loop() {
  handleBleEvents();
}
