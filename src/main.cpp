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
  constexpr int SERIAL_BAUD_RATE           = 115200;
  constexpr int CLIENT_CONNECT_TIMEOUT     = 30;
  constexpr int SCAN_TIMEOUT_SECONDS       = 30;
  constexpr uint64_t TEST_SERVER_ADDRESS   = 0x7821847C1C52;
  constexpr uint64_t REAL_SERVER_ADDRESS   = 0xF797AC1FF8C0;
  constexpr uint64_t IPHONE_CLIENT_ADDRESS = 0xC02C5C83709A;
  constexpr int CONNECTION_INTERVAL_MIN    = 12;
  constexpr int CONNECTION_INTERVAL_MAX    = 12;
  constexpr int SUPERVISION_TIMEOUT        = 52;
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

  int gapHandler(ble_gap_event *event, void * /* arg */) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    switch (event->type) {
    case BLE_GAP_EVENT_MTU:
      xSemaphoreGiveFromISR(utility::semaphore, &xHigherPriorityTaskWoken);
    }

    if (xHigherPriorityTaskWoken == pdTRUE) {
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    return 0;
  }

  bool subscribeToCharacteristic(NimBLEClient * /* pClient */, NimBLERemoteCharacteristic *chr) {
    if (!chr->getUUID().equals(charUUID)) {
      Log.warningln("\t\t\tCharacteristic UUID does not match, skipping");
      return false;
    }

    Log.traceln("\t\t\tWill try to subscribe to characteristic");
    if (chr->canNotify() && chr->subscribe(true, onEvent)) {
      Log.noticeln("\t\t\tSuccessfully subscribed to characteristic using notify");
      return true;
    }

    if (chr->canIndicate() && chr->subscribe(false, onEvent)) {
      Log.noticeln("\t\t\tSuccessfully subscribed to characteristic using indicate");
      return true;
    }

    Log.errorln("\t\t\tCharacteristic cannot notify or indicate");
    return false;
  }

  bool subscribeToService(NimBLEClient *pClient, NimBLERemoteService *pService) {
    if (!pService->getUUID().equals(serviceUUID)) {
      Log.warningln("\t\tInvalid service UUID (%s)", serviceUUID);
      return false;
    }

    Log.traceln("\t\tSearch for characteristic by UUID");
    auto pChar = pService->getCharacteristic(charUUID);
    if (pChar && subscribeToCharacteristic(pClient, pChar)) {
      return true;
    }

    Log.traceln("\t\tWill go tru all characteristics (no reload)");
    for (auto pChar: *pService->getCharacteristics(false)) {
      if (subscribeToCharacteristic(pClient, pChar)) {
        return true;
      }
    }

    Log.noticeln("\t\tWill go tru all characteristics (reload)");
    for (auto pChar: *pService->getCharacteristics(true)) {
      if (subscribeToCharacteristic(pClient, pChar)) {
        return true;
      }
    }

    Log.errorln("\t\tCould not find characteristic by UUID");
    return false;
  }

  bool subscribeToClient(NimBLEClient *pClient) {
    Log.traceln("\tFind service by UUID (%s)", serviceUUID);
    auto service = pClient->getService(serviceUUID);
    if (service && subscribeToService(pClient, service)) {
      return true;
    }

    Log.traceln("\tWill go tru all services (no reload)");
    for (auto pService: *pClient->getServices(false)) {
      if (subscribeToService(pClient, pService)) {
        return true;
      }
    }

    Log.warningln("\tCould not find service by UUID (no reload)");
    Log.noticeln("\tWill go tru all services (reload)");
    for (auto pService: *pClient->getServices(true)) {
      if (subscribeToService(pClient, pService)) {
        return true;
      }
    }

    Log.errorln("\tCould not find service by UUID");
    return false;
  }
  void setup() {
    initArduino();

    Serial.begin(SERIAL_BAUD_RATE);
    Log.begin(LOG_LEVEL_NOTICE, &Serial, true);

    removeWatchdog();

    Serial.println("Starting ESP32 Proxy @ " + String(GIT_COMMIT));

    NimBLEDevice::init(utility::DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
    NimBLEDevice::setCustomGapHandler(gapHandler);

    Log.infoln("Starting broadcasting BLE keyboard");
    utility::keyboard.begin();

    Log.infoln("[SEM] Wait for iPhone to complete MTU exchange");
    auto currTime = millis();
    xSemaphoreTake(utility::semaphore, portMAX_DELAY);
    Log.infoln("[SEM] Waited %d ms for iPhone to complete MTU exchange", millis() - currTime);

    NimBLEDevice::whiteListAdd(testServerAddress);
    NimBLEDevice::whiteListAdd(realServerAddress);

    removeWatchdog();

    Log.traceln("Starting BLE scan for the Terrain Command");
    auto *pScan                      = NimBLEDevice::getScan();
    auto *pAdvertisedDeviceCallbacks = new AdvertisedDeviceCallbacks();

    pScan->setAdvertisedDeviceCallbacks(pAdvertisedDeviceCallbacks, true);
    pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
    pScan->setActiveScan(false);
    pScan->setMaxResults(1);
    pScan->setInterval(100);
    pScan->setWindow(99);

    auto results = pScan->start(SCAN_TIMEOUT_SECONDS);
    if (results.getCount() == 0) {
      utility::reboot("Could not find the Terrain Command");
    }

    auto device = results.getDevice(0);
    auto addr     = device.getAddress();
    auto pClient  = llvm_libc::createBLEClient(addr);
    auto clientCb = new ClientCallbacks();

    pClient->setClientCallbacks(clientCb, true);
    pClient->setConnectTimeout(CLIENT_CONNECT_TIMEOUT);
    pClient->setConnectionParams(CONNECTION_INTERVAL_MIN, CONNECTION_INTERVAL_MAX, 0, SUPERVISION_TIMEOUT);

    updateWatchdogTimeout(WATCHDOG_TIMEOUT_3);

    auto currTime2 = millis();
    Log.noticeln("[SEM2] Wait for Terrain Command to complete MTU exchange");
    if (!pClient->connect()) {
      utility::reboot("Could not connect to the Terrain Command");
    }

    Log.noticeln("[SEM2] Wait for Terrain Command to complete MTU exchange");
    xSemaphoreTake(utility::semaphore, portMAX_DELAY);
    Log.noticeln("[SEM2] Waited %d ms for Terrain Command to complete MTU exchange", millis() - currTime2);

    vSemaphoreDelete(utility::semaphore);
    updateWatchdogTimeout(WATCHDOG_TIMEOUT_4);

    auto currTime3 = millis();
    if (!subscribeToClient(pClient)) {
      disconnect(pClient, "Could not subscribe to Terrain Command");
    } else {
      Log.noticeln("Successfully subscribed to Terrain Command in %d ms", millis() - currTime3);
    }

    removeWatchdog();
  }
} // namespace llvm_libc

extern "C" void init_arduino() {
  utility::enableLED();
  utility::ledon();
  llvm_libc::setup();
  utility::ledoff();
}
