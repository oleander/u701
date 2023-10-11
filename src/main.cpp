#include <ArduinoLog.h>
#include <WiFi.h>
/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR
#define WDT_TIMEOUT 10 * 60

#include "ClientCallback.h"
#include "ffi.h"
#include "keyboard.h"
#include "ota.h"
#include "settings.h"
#include "shared.h"
#include "utility.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>

IPAddress ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

constexpr auto COMMAND_MAP_SERVICE_UUID = "19B10010-E8F2-537E-4F6C-D104768A1214";
constexpr auto COMMAND_MAP_CHAR_UUID    = "19B10011-E8F2-537E-4F6C-D104768A1214";
constexpr auto COMMAND_SERVICE_UUID     = "19B10000-E8F2-537E-4F6C-D104768A1214";
constexpr auto COMMAND_CHAR_UUID        = "19B10001-E8F2-537E-4F6C-D104768A1214";

const auto address = NimBLEAddress(DEVICE_MAC, 1);

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

const auto RESTART_CMD = "restart";
const auto UPDATE_CMD  = "update";

class MyCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *characteristic) override {
    std::string cmd = characteristic->getValue();
    if (cmd.length() == 0) return;
    Log.noticeln("Received value: %s\n", cmd.c_str());

    if (cmd == RESTART_CMD) {
      restart("Restart command received");
    } else if (cmd == UPDATE_CMD) {
      characteristic->setValue("Toggle OTA");
      state.action = Action::INIT_OTA;
      auto scan    = NimBLEDevice::getScan();
      if (scan->isScanning()) scan->stop();
    } else {
      characteristic->setValue("Unknown command");
    }
  }
};

/* Add function isActive to the State struct */
static void onEvent(BLERemoteCharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify) {
  Log.noticeln("Received data: %s\n", data);
  Log.noticeln("Received length: %d\n", length);
  Log.noticeln("Received isNotify: %d\n", isNotify);

  transition_from_cpp(data, length);
}

void setupKeyboard() {
  Log.noticeln("Enable Keyboard");
  keyboard.begin();
  keyboard.setDelay(13);
}

void setupSerial() {
  Serial.begin(SERIAL_BAUD_RATE);
#ifdef RELEASE
  Log.begin(LOG_LEVEL_SILENT, &Serial);
#else
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
#endif

  Log.noticeln("Starting ESP32 ...");
}

/**
 * Sets up the client to connect to the BLE device with the specified MAC address.
 * If the connection fails or no services/characteristics are found, the device will restart.
 */
void setupClient() {
  if (!device) {
    restart("Device not found, will reboot");
  }

  client = NimBLEDevice::createClient();
  client->setClientCallbacks(new ClientCallback());

  Log.noticeln("Connecting to %s ...", address.toString().c_str());
  if (!client->connect(device)) {
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
    if (advertised->getAddress() == address) {
      Log.noticeln("Found device device");
      device = advertised;
      advertised->getScan()->stop();
    } else {
      Log.noticeln("Found device %s", advertised->getAddress().toString().c_str());
    }
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

void setupBLE() {
  Log.noticeln("Starting BLE server...");

  NimBLEDevice::init(DEVICE_NAME);

  server   = NimBLEDevice::createServer();
  service1 = server->createService(COMMAND_SERVICE_UUID);
  char1    = service1->createCharacteristic(COMMAND_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

  char1->setCallbacks(new MyCallbacks());
  service1->start();

  service2 = server->createService(COMMAND_MAP_SERVICE_UUID);
  char2    = service2->createCharacteristic(COMMAND_MAP_CHAR_UUID, NIMBLE_PROPERTY::READ);
  char2->setValue("Available commands:restart update");
  service2->start();

  advert = NimBLEDevice::getAdvertising();
  advert->addServiceUUID(COMMAND_SERVICE_UUID);
  advert->addServiceUUID(COMMAND_MAP_SERVICE_UUID);
  advert->start();
}

void setup() {
  setupSerial();
  setupBLE();
  setup_rust();
  setupKeyboard();
  setupScan();
  setupClient();

  WiFi.config(ip, gateway, subnet);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, 1, true);

  ArduinoOTA.setPassword(WIFI_PASSWORD);
  ArduinoOTA.begin();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Log.error("WiFi disconnected, restarting...");
    delay(1000);
    ESP.restart();
  }

  ArduinoOTA.handle();
}
