#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#include "ClientCallback.h"
#include "keyboard.h"
#include "settings.h"
#include "shared.h"
#include "utility.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
#include <OneButton.h>

const auto address      = NimBLEAddress(MAC_ADRESS, 1);
const int SEVEN_MINUTES = 420000000;
static bool activeState = false;
static OneButton *activeButton;
static bool otaEnabled = false;
hw_timer_t *timer      = NULL;

/* Called when a notification is received from the BLE device, i.e a button press/release  */
static void onNotification(BLERemoteCharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify) {
  if (characteristic->getUUID() != reportUUID) return;
  if (length != 4) return;
  if (!isNotify) return;

  auto currentID = dataToInt(data, length);

  if (currentID == 0x0000) { // Button was released
    activeState = false;
  } else if (!activeState) { // Button was pressed, and another button is not already pressed
    activeButton = buttons.at(currentID);
    activeState  = true;
  }

  /* Ensure that the button callbacks are fired at least once */
  if (activeButton) {
    activeButton->tick(activeState);
  }

#ifdef DEBUG
  Log.noticeln("Event %i received", characteristic->getUUID().toString().c_str());
#endif
}

void setupKeyboard() {
  Log.noticeln("Enable Keyboard");
  keyboard.begin();
}

/* True if OTA should be activated */
bool willActivateOTA() {
  return activeState && !keyboard.isConnected() && !otaEnabled;
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
  client = NimBLEDevice::createClient(address);
  client->setClientCallbacks(new ClientCallback());

  Log.noticeln("Connecting to %s ...", address.toString().c_str());
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

      auto status = characteristic->subscribe(true, onNotification, true);
      if (!status) {
        restart("[BUG] Failed to subscribe to notifications");
      }

      Log.noticeln("Subscribed to notifications");
      return;
    }
  }

  restart("[BUG] No report characteristic found");
}

/**
 * Interrupt service routine that is triggered by a timer after seven minutes.
 * Calls the restart function with the message "OTA update failed" and false as the second argument.
 */
void IRAM_ATTR onTimer() {
  restart("OTA update failed");
}

/**
 * Sets up a timer to trigger an interrupt after seven minutes.
 * The interrupt will call the onTimer function.
 * Used to reboot the ESP32 after seven minutes of OTA.
 */
void setupTimer() {
  timer = timerBegin(0, 40, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, SEVEN_MINUTES, false);
}

class Callbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertised) {
    if (advertised->getAddress() == address) {
#ifdef DEBUG
      Log.noticeln("Found device device");
#endif

      advertised->getScan()->stop();
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

void setup() {
  setupSerial();
  setupButtons();
  setupKeyboard();
  setupScan();
  setupClient();
  setupTimer();
}

void loop() {
  if (willActivateOTA()) { /* Handle OTA */
    Log.noticeln("Activating OTA");

    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
    ArduinoOTA.setRebootOnSuccess(true);
    ArduinoOTA.begin();
    otaEnabled = true;

    /* Will reboot the ESP32 after 7 minutes */
    timerAlarmEnable(timer);
  } else if (otaEnabled) {
    ArduinoOTA.handle();
  } else if (activeButton) { /* Handle button presses */
    activeButton->tick(activeState);
  }

  delay(20);
};
