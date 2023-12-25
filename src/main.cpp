
#include "ffi.h"
#include "settings.h"

#include <ArduinoLog.h>
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
// #include <esp_task_wdt.h>

// #define WDT_TIMEOUT 30

// IPAddress ip(192, 168, 4, 1);
// IPAddress gateway(192, 168, 4, 1);
// IPAddress subnet(255, 255, 255, 0);

static NimBLEUUID reportUUID("2A4D");
static NimBLEUUID cccdUUID("2902");
static NimBLEUUID hidService("1812");
static NimBLEUUID batteryServiceUUID("180F");
static NimBLEUUID batteryLevelCharUUID("2A19");

static NimBLEClient *client;

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

void restart(const char *reason) {
  Log.noticeln(reason);
  Log.noticeln("Restarting ESP32 in 5 seconds ...");
  delay(5000);
  ESP.restart();
}

/* Add function isActive to the State struct */
static void handleButtonClick(BLERemoteCharacteristic *_, uint8_t *data, size_t length, bool isNotify) {
  if (!isNotify) {
    return;
  }

  c_on_event(data, length);
}

class ClientCallback : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *client) override {
    Log.noticeln("Discovering services ...");
    for (auto &service: *client->getServices(true)) {
      Log.noticeln("Discovering characteristics ...");
      for (auto &characteristic: *service->getCharacteristics(true)) {
        auto currentServiceUUID = service->getUUID().toString().c_str();
        auto currentCharUUID    = characteristic->getUUID().toString().c_str();

        if (!service->getUUID().equals(hidService)) {
          Log.warningln("[Click] Unknown report service: %X", currentServiceUUID);
        } else if (!characteristic->getUUID().equals(reportUUID)) {
          Log.warningln("[Click] Unknown report characteristic: %X", currentCharUUID);
        } else if (!characteristic->canNotify()) {
          Log.warningln("[Click] Cannot subscribe to notifications: %X", currentCharUUID);
        } else if (!characteristic->subscribe(true, handleButtonClick, false)) {
          Log.errorln("[Click] [Bug] Failed to subscribe to notifications: %X", currentCharUUID);
        } else {
          return Log.noticeln("[Click] Subscribed to notifications: %X", currentCharUUID);
        }
      }
    }

    Log.noticeln("Could not subscribe to notifications");
  }

  void onDisconnect(NimBLEClient *client) override {
    restart("Disconnected from device");
  }
};

static auto buttonMacAddress = NimBLEAddress(DEVICE_MAC, 1);
static auto scan             = NimBLEDevice::getScan();
static auto clientCallback   = ClientCallback();

class ScanCallback : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertised) {
    auto macAddr = advertised->getAddress();

    if (macAddr != buttonMacAddress) {
      return;
    }

    Log.noticeln("Found device, will try to connect");
    client = NimBLEDevice::createClient(macAddr);

    client->setClientCallbacks(&clientCallback);
    if (client->isConnected()) {
      return Log.noticeln("Already connected to device");
    }

    if (!client->connect()) {
      restart("Could not connect to the Terrain Command");
    }

    Log.noticeln("Stop scan");
    advertised->getScan()->stop();
  }
};

static auto scanCallback = ScanCallback();

BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);

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

void initializeKeyboard() {
  Log.noticeln("Enable Keyboard");

  keyboard.setBatteryLevel(100);
  keyboard.setDelay(12);
  keyboard.begin();
}

void initializeSerialCommunication() {
  printf("Starting ESP32 ...\n");
  Serial.begin(SERIAL_BAUD_RATE);
  Log.begin(LOG_LEVEL_MAX, &Serial);
  Log.setLevel(LOG_LEVEL_MAX);
  Log.noticeln("Starting ESP32 ...");
}

void startBLEScanForDevice() {
  Log.noticeln("Starting BLE scan ...");

  scan->setAdvertisedDeviceCallbacks(&scanCallback);
  scan->setInterval(SCAN_INTERVAL);
  scan->setWindow(SCAN_WINDOW);
  scan->setActiveScan(true);
  scan->setMaxResults(0);
  scan->start(0, false);
}

extern "C" void init_arduino() {
  initializeSerialCommunication();
  startBLEScanForDevice();
  initializeKeyboard();
}
