#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#include "ClientCallback.h"
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

const int TEN_MINUTES   = 10 * 60 * 1000;
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
  auto address = NimBLEAddress(MAC_ADRESS, 1);
  client       = NimBLEDevice::createClient(address);

  client->setClientCallbacks(new ClientCallback());
  client->setConnectTimeout(15);

  if (!client->connect()) {
    restart("Timeout connecting to the device", false);
  }

  Log.noticeln("Connecting to the service ...");
  auto service = client->getService(hidService);
  if (!service) {
    restart("Service not found, will retry", false);
  }

  Log.noticeln("Getting the characteristic ...");
  auto characteristic = service->getCharacteristic(reportUUID);
  if (!characteristic) {
    restart("Characteristic not found, will retry", false);
  }

  auto status = characteristic->subscribe(true, onNotification, true);
  if (!status) {
    restart("Could not subscribe to notifications", false);
  }

  Log.noticeln("Connected to the service");
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
  if (otaStatus != 1) return;
  Log.noticeln("Enable OTA");
  setupOTA();
}

void setup() {
  setupSerial();
  setupOTAStatus();
  setupButtons();
  setupKeyboard();
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
