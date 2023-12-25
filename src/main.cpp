
// A BLE proxy
// 1. Scans for a BLE device
// 2. Connects to the device
// 3. Subscribes to the device
// 4. Forwards events to the host

// The mapping is done elsewhere

#include "ffi.h" // Ignore

#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>

static NimBLEUUID reportUUID("2A4D");
static NimBLEUUID hidService("1812");
static NimBLEUUID cccdUUID("2902");

// #define DEVICE_MAC          "A8:42:E3:CD:FB:C6"
#define DEVICE_MAC          "f7:97:ac:1f:f8:c0"
#define SCAN_INTERVAL       500 // in ms
#define SCAN_WINDOW         450 // in ms
#define DEVICE_NAME         "u701"
#define SERIAL_BAUD_RATE    115200
#define DEVICE_BATTERY      100
#define DEVICE_MANUFACTURER "u701"

BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

/* Event received from the Terrain Command */
static void handleButtonClick(BLERemoteCharacteristic *_, uint8_t *data, size_t length, bool isNotify) {
  if (!isNotify) {
    return;
  }

  c_on_event(data, length);
}

// Terrain Command callbacks
class ClientCallback : public NimBLEClientCallbacks {
  // When client is connected:
  // 1. Find the HID service
  // 2. Find the report characteristic
  // 3. Subscribe to the report characteristic
  void onConnect(NimBLEClient *client) override {
    // TODO: Stop the scanner here?
    for (auto &service: *client->getServices(false)) { // Do we need to reload these values or does the cache work?
      for (auto &characteristic: *service->getCharacteristics(false)) {
        if (!service->getUUID().equals(hidService)) {
          continue;
        } else if (!characteristic->getUUID().equals(reportUUID)) {
          continue;
        } else if (!characteristic->canNotify()) {
          continue;
          // What does {true} do?
        } else if (!characteristic->subscribe(true, handleButtonClick, false)) {
          continue;
        } else {
          return;
        }
      }
    }
  }

  // When client is disconnected, restart the ESP32
  void onDisconnect(NimBLEClient *client) override {
    // Question: Is there a way to tell the system why we restart?
    ESP.restart();
  }
};

static auto buttonMacAddress = NimBLEAddress(DEVICE_MAC, 1);
static auto scan             = NimBLEDevice::getScan();
static auto clientCallback   = ClientCallback();

// Search for client
class ScanCallback : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertised) {
    auto macAddr = advertised->getAddress();

    if (macAddr != buttonMacAddress) {
      return;
    }

    // Is it a problem that {client} is not freed?
    auto client = NimBLEDevice::createClient(macAddr);
    client->setClientCallbacks(&clientCallback);

    // Connect to the client
    // Should we stop before this?
    if (!client->connect()) {
      advertised->getScan()->stop();
    }
  }
};

static auto scanCallback = ScanCallback();

extern "C" void init_arduino() {
  Serial.begin(SERIAL_BAUD_RATE);

  Serial.println("Starting BLE scan");
  // Question: Does scan have to be global?
  scan->setAdvertisedDeviceCallbacks(&scanCallback);
  scan->setInterval(SCAN_INTERVAL);
  scan->setWindow(SCAN_WINDOW);
  scan->setActiveScan(true);
  scan->setMaxResults(0);
  scan->start(0, false);

  Serial.println("Starting BLE keyboard");
  NimBLEDevice::init(DEVICE_NAME);
  keyboard.begin();
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
