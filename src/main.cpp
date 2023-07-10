#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#include "ClientCallback.h"
#include "ScanCallback.h"
#include "keyboard.h"
#include "ota.h"
#include "settings.h"
#include "shared.h"
#include "utility.h"
#include <Arduino.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
#include <OneButton.h>

static int TEN_MINUTES  = 10 * 60 * 1000;
static bool activeState = false;
static OneButton *activeButton;

static void onNotification(BLERemoteCharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify) {
  if (!isNotify) return;
  if (length != 4) return;

  auto currentID = dataToInt(data, length);

  if (currentID == 0x0000) { // Button was released
    activeState = false;
  } else if (!activeState) { // Button was pressed, and another button is not already pressed
    activeButton = buttons.at(currentID);
    activeState  = true;
  }

  if (activeButton) {
    activeButton->tick(activeState);
  }
}

void scanForDevice() {
  auto scan = NimBLEDevice::getScan();

  scan->setAdvertisedDeviceCallbacks(new ScanCallback());
  scan->setInterval(SCAN_INTERVAL);
  scan->setWindow(SCAN_WINDOW);
  scan->start(0, false);
  scan->clearResults();
}

void setupKeyboard() {
  Log.noticeln("Enable Keyboard");
  keyboard.begin();
  keyboard.setDelay(12);
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

void setupClient() {
  client = NimBLEDevice::createClient();

  client->setClientCallbacks(new ClientCallback());
  client->connect(device);

  Log.noticeln("Discovering services ...");
  auto services = client->getServices(true);
  if (services->empty()) {
    restart("No services found, will retry", false);
  }

  for (auto &service: *services) {
    if (!service->getUUID().equals(hidService)) continue;

    Log.noticeln("Discovering characteristics ...");
    auto characteristics = service->getCharacteristics(true);

    if (characteristics->empty()) {
      restart("[BUG] No characteristics found", false);
    }

    for (auto &characteristic: *characteristics) {
      if (!characteristic->getUUID().equals(reportUUID)) continue;
      if (!characteristic->canNotify()) continue;

      Log.noticeln("Trying to subscribe to notifications ...");
      auto status = characteristic->subscribe(true, onNotification, true);
      if (!status) {
        restart("Could not subscribe to notifications", false);
      }

      Log.noticeln("Ready to receive notifications from buttons!");
    }
  }
}

void handleClickEvent() {
  if (activeButton) {
    activeButton->tick(activeState);
  }

  /* Toggle OTA mode */
  if (activeState && !keyboard.isConnected()) {
    restart("Toggle OTA", true);
  }
}

void setupOTAStatus() {
  esp_reset_reason_t reason = esp_reset_reason();

  if (reason == ESP_RST_POWERON || reason == ESP_RST_SW || reason == ESP_RST_BROWNOUT) {
    otaStatus = 0;
  } else if (otaStatus) {
    Log.noticeln("Enable OTA");
    setupOTA();
  }
}

void setup() {
  setupSerial();
  setupOTAStatus();
  setupButtons();
  setupKeyboard();
  delay(1000);
  scanForDevice();
  setupClient();
}

void loop() {
  if (otaStatus && millis() < TEN_MINUTES) {
    handleOTA();
  } else if (otaStatus) {
    restart("OTA mode enabled for more 10 minutes, will restart", false);
  } else {
    handleClickEvent();
  }
};
