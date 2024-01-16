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

#define OTA_WIFI_SSID "u701"
#define OTA_WIFI_PASS "11111111"

namespace llvm_libc {
  constexpr int SERIAL_BAUD_RATE       = 115200;
  constexpr int CLIENT_CONNECT_TIMEOUT = 30;
  // constexpr uint64_t TEST_SERVER_ADDRESS   = 0x083A8D9A444A;
  // 78:21:84:7C:1C:52
  constexpr uint64_t TEST_SERVER_ADDRESS   = 0x7821847C1C52;
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

  void resetWatchdog() {
    Log.traceln("Reset watchdog");
    esp_task_wdt_reset();
  }

  template <typename... Args> void disconnect(NimBLEClient *pClient, const char *format, Args &&...args) {
    Log.traceln("Disconnect from Terrain Command");
    pClient->disconnect();
    utility::reboot(std::string(format), std::forward<Args>(args)...);
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

  int gapHandler(ble_gap_event *event, void * /* arg */) {
    switch (event->type) {
    case BLE_GAP_EVENT_DISCONNECT:
      utility::reboot("[BLE_GAP_EVENT_DISCONNECT] Someone disconnected");
    case BLE_GAP_EVENT_MTU:
      Log.info("[BLE_GAP_EVENT_MTU] Release semaphore");
      xSemaphoreGive(utility::semaphore);
      break;
    default:
      Log.traceln("Unknown GAP event: %d", event->type);
      break;
    }

    return event->type;
  }

  void setup() {
    initArduino();

    Serial.begin(SERIAL_BAUD_RATE);
    Log.begin(LOG_LEVEL_MAX, &Serial, true);

    removeWatchdog();

    NimBLEDevice::init(utility::DEVICE_NAME);

    Serial.println("Starting ESP32 Proxy @ " + String(GIT_COMMIT));

    Log.infoln("Broadcasting BLE keyboard");
    utility::keyboard.begin(&iPhoneClientAddress);

    NimBLEDevice::setPower(ESP_PWR_LVL_N12);
    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_SC | BLE_SM_PAIR_AUTHREQ_MITM);
    NimBLEDevice::setCustomGapHandler(gapHandler);

    Log.traceln("[SEM] Wait for iPhone to complete MTU exchange");
    xSemaphoreTake(utility::semaphore, portMAX_DELAY);

    NimBLEDevice::whiteListAdd(testServerAddress);
    NimBLEDevice::whiteListAdd(realServerAddress);
    // NimBLEDevice::setMTU(44);

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

    Log.noticeln("[SEM] Wait TC to complete MTU exchange");
    xSemaphoreTake(utility::semaphore, portMAX_DELAY);

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
