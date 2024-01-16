#include <Arduino.h>
#include <ArduinoLog.h>
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <esp_task_wdt.h>

#include <array>
#include <iostream>
#include <string>
#include <vector>

#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "AdvertisedDeviceCallbacks.hh"
#include "ClientCallbacks.hh"
#include "ffi.hh"
#include "utility.h"

#define SSID          "boat"
#define PASS          "0304673428"
#define OTA_WIFI_SSID "u701"

namespace llvm_libc {
  constexpr int SERIAL_BAUD_RATE           = 115200;
  constexpr int CLIENT_CONNECT_TIMEOUT     = 30;
  constexpr uint64_t TEST_SERVER_ADDRESS   = 0x083A8D9A444A;
  constexpr uint64_t REAL_SERVER_ADDRESS   = 0xF797AC1FF8C0;
  constexpr uint64_t IPHONE_CLIENT_ADDRESS = 0xC02C5C83709A;
  constexpr int CONNECTION_INTERVAL_MIN    = 12;
  constexpr int CONNECTION_INTERVAL_MAX    = 12;
  constexpr int SUPERVISION_TIMEOUT        = 51;
  constexpr int WATCHDOG_TIMEOUT_1         = 120;
  constexpr int WATCHDOG_TIMEOUT_2         = 5 * 60;
  constexpr int WATCHDOG_TIMEOUT_3         = 60;
  constexpr int WATCHDOG_TIMEOUT_4         = 20;

  const std::array<char, 5> SERVICE_UUID = {"1812"};
  const std::array<char, 5> CHAR_UUID    = {"2a4d"};

  const NimBLEAddress testServerAddress(TEST_SERVER_ADDRESS, BLE_ADDR_PUBLIC); // TEST
  const NimBLEAddress realServerAddress(REAL_SERVER_ADDRESS, BLE_ADDR_RANDOM); // REAL
  const NimBLEUUID serviceUUID(SERVICE_UUID.data());
  const NimBLEUUID charUUID(CHAR_UUID.data());

  NimBLEAddress iPhoneClientAddress(IPHONE_CLIENT_ADDRESS, BLE_ADDR_RANDOM);

  NimBLEClient *createBLEClient(const NimBLEAddress &addr) {
    return NimBLEDevice::createClient(addr);
  }

  void onEvent(NimBLERemoteCharacteristic * /* pRemoteCharacteristic */, uint8_t *data, size_t length, bool isNotify) {
    if (!isNotify) {
      return;
    }

    c_on_event(data, length);
  }

  void updateWatchdogTimeout(uint32_t newTimeoutInSeconds) {
    Log.traceln("Update watchdog timeout to %d seconds", newTimeoutInSeconds);
    esp_task_wdt_deinit();
    esp_task_wdt_init(newTimeoutInSeconds, true);
    esp_task_wdt_add(nullptr);
  }

  void removeWatchdog() {
    Log.traceln("Remove watchdog");
    esp_task_wdt_delete(nullptr);
    esp_task_wdt_deinit();
  }

  void onClientDisconnect(NimBLEServer * /* _server */) {
    utility::reboot("Client disconnected from the keyboard");
  }

  template <typename... Args> void disconnect(NimBLEClient *pClient, const char *format, Args &&...args) {
    Log.traceln("Disconnect from Terrain Command");
    pClient->disconnect();
    utility::reboot(std::string(format), std::forward<Args>(args)...);
  }

  auto onClientConnect(ble_gap_conn_desc * /* _desc */) -> void {
    Log.traceln("Connected to keyboard");
    Log.traceln("Release keyboard semaphore (output) (semaphore)");
    xSemaphoreGive(utility::outgoingClientSemaphore);
  }

  bool subscribeToCharacteristic(NimBLEClient * /* pClient */, NimBLERemoteCharacteristic *chr) {
    if (!chr->getRemoteService()->getUUID().equals(serviceUUID)) {
      Log.traceln("Service UUID does not match, skipping");
      return false;
    }

    if (!chr->getUUID().equals(charUUID)) {
      Log.traceln("Characteristic UUID does not match, skipping");
      return false;
    }

    if (chr->canNotify() && chr->subscribe(true, onEvent)) {
      Log.noticeln("Successfully subscribed to characteristic (notify)");
      return true;
    }

    if (chr->canIndicate() && chr->subscribe(false, onEvent)) {
      Log.noticeln("Successfully subscribed to characteristic (indicate))");
      return true;
    }

    Log.warningln("Characteristic cannot notify or indicate, skipping");
    return false;
  }

  bool subscribe(NimBLEClient *pClient, bool cache) {
    for (auto pService: *pClient->getServices(cache)) {
      for (auto pChar: *pService->getCharacteristics(cache)) {
        if (subscribeToCharacteristic(pClient, pChar)) {
          return true;
        }
      }
    }

    return false;
  }

  const int wifiConnectDelayMs     = 500;
  const int watchdogTimeoutSeconds = 3;
  TaskHandle_t Task1;

  void setupArduinoOTA(void * /* parameter */) {
    removeWatchdog();

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      vTaskDelay(200);
    }

    ArduinoOTA.setHostname(OTA_WIFI_SSID);
    ArduinoOTA.setRebootOnSuccess(true);
    ArduinoOTA.onError([](ota_error_t error) { utility::reboot("Error[%u]: ", error); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      static unsigned int lastProgress = 0;
      if (progress != lastProgress) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        lastProgress = progress;
      }
    });

    ArduinoOTA.begin();

    while (true) {
      ArduinoOTA.handle();
      vTaskDelay(10);
    }
  }

  void setup() {
    initArduino();

    Serial.begin(SERIAL_BAUD_RATE);
    Log.begin(LOG_LEVEL_MAX, &Serial, true);

    Log.noticeln("Starting setupArduinoOTA (ok)");
    // core 1
    xTaskCreatePinnedToCore(setupArduinoOTA, "setupArduinoOTA", 10000, NULL, 0, &Task1, 0);

    removeWatchdog();

    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
    NimBLEDevice::init(utility::DEVICE_NAME);

    Serial.println("Starting ESP32 Proxy @ " + String(GIT_COMMIT));

    Log.infoln("Broadcasting BLE keyboard");
    utility::keyboard.whenClientConnects(onClientConnect);
    utility::keyboard.whenClientDisconnects(onClientDisconnect);
    utility::keyboard.begin(&iPhoneClientAddress);

    Log.traceln("Wait for the keyboard to connect (output) (semaphore)");
    xSemaphoreTake(utility::outgoingClientSemaphore, portMAX_DELAY);

    NimBLEDevice::whiteListAdd(testServerAddress);
    NimBLEDevice::whiteListAdd(realServerAddress);

    removeWatchdog();

    Log.traceln("Starting BLE scan for the Terrain Command");
    auto *pScan                      = NimBLEDevice::getScan();
    auto *pAdvertisedDeviceCallbacks = new AdvertisedDeviceCallbacks();
    pScan->setAdvertisedDeviceCallbacks(pAdvertisedDeviceCallbacks, true);
    pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
    pScan->setActiveScan(true);
    pScan->setMaxResults(1);

    auto results = pScan->start(0);
    if (!results.getCount()) {
      utility::reboot("Could not find the Terrain Command");
    }

    auto device   = results.getDevice(0);
    auto addr     = device.getAddress();
    auto pClient  = llvm_libc::createBLEClient(addr);
    auto clientCb = new ClientCallbacks();

    pClient->setClientCallbacks(clientCb, true);
    pClient->setConnectTimeout(CLIENT_CONNECT_TIMEOUT);
    pClient->setConnectionParams(CONNECTION_INTERVAL_MIN, CONNECTION_INTERVAL_MAX, 0, SUPERVISION_TIMEOUT);

    updateWatchdogTimeout(WATCHDOG_TIMEOUT_3);
    if (!pClient->connect()) {
      utility::reboot("Could not connect to the Terrain Command");
    }

    Log.noticeln("Wait for the Terrain Command to authenticate (input) (semaphore)");
    xSemaphoreTake(utility::incommingClientSemaphore, portMAX_DELAY);

    updateWatchdogTimeout(WATCHDOG_TIMEOUT_4);
    Log.noticeln("Try subscribing to existing services & characteristics");
    if (subscribe(pClient, false)) {
      Log.noticeln("Subscribed to existing services & characteristics");
      return removeWatchdog();
    } else {
      Log.warningln("Could not subscribe to existing services & characteristics");
      if (subscribe(pClient, true)) {
        Log.noticeln("Subscribed to discovered services & characteristics");
        return removeWatchdog();
      } else {
        disconnect(pClient, "Could not subscribe to discovered services & characteristics");
      }
    }
  }
} // namespace llvm_libc

extern "C" void init_arduino() {
  llvm_libc::setup();
}
