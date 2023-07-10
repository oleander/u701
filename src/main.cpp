#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#include "keyboard.h"
#include "ota.h"
#include "settings.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
#include <OneButton.h>

static NimBLEUUID hidService("1812");
static NimBLEUUID reportUUID("2A4D");
static NimBLEUUID cccdUUID("2902");

static int TEN_MINUTES  = 10 * 60 * 1000;
static uint8_t ON[]     = {0x1, 0x0};
static bool activeState = false;
static int otaStatus    = 0;

OneButton *activeButton;

int32_t dataToInt(uint8_t *pData, size_t length) {
  int32_t result = 0;

  for (size_t i = 0; i < length; ++i) {
    result = (result << 8) | pData[i];
  }

  return result;
}

static void onNotification(BLERemoteCharacteristic *characteristic, uint8_t *data, size_t length,
                           bool isNotify) {
  if (!isNotify) return;
  if (length != 4) return;

  auto currentID = dataToInt(data, length);

  Log.noticeln("Got notification from ID 0x%x\n", currentID);

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

void restart(const char *reason) {
  Log.noticeln("Restarting ESP32 ...");
  Log.noticeln(reason);
  delay(5000);
  ESP.restart();
}

class ClientCallback : public NimBLEClientCallbacks {
  void onConnect(BLEServer *pServer) { Log.noticeln("Connected to device!"); }

  void onDisconnect(NimBLEClient *client) {
    restart("Disconnected from device, restarting ESP32 ...");
  }
};

NimBLEAdvertisedDevice *device;

class Callbacks : public NimBLEAdvertisedDeviceCallbacks {
public:
  void onResult(NimBLEAdvertisedDevice *advertised) {
    Log.notice(".");

    if (!advertised->isAdvertisingService(hidService)) return;

    auto addr = advertised->getAddress().toString();
    if (strcmp(addr.c_str(), DEVICE_MAC) != 0) return;

    auto name = advertised->getName();
    auto rssi = advertised->getRSSI();
    auto manu = advertised->getManufacturerData();
    auto serv = advertised->getServiceData();

    Log.noticeln("Found device: %s (%s) RSSI=%d, manu=%d, serv=%d\n", name.c_str(), addr.c_str(),
                 rssi, manu.size(), serv.size());

    Log.noticeln("Connecting to advertised device ...");
    device = advertised;

    Log.noticeln("Stopping scan ...");
    advertised->getScan()->stop();
  }
};

void scanForDevice() {
  auto scan = NimBLEDevice::getScan();

  scan->setAdvertisedDeviceCallbacks(new Callbacks());
  scan->setInterval(SCAN_INTERVAL);
  scan->setWindow(SCAN_WINDOW);
  scan->setActiveScan(true);
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
}

NimBLEClient *client;

void setupClient() {
  client = NimBLEDevice::createClient();

  client->setClientCallbacks(new ClientCallback());
  client->connect(device);

  Log.noticeln("Discovering services ...");
  auto sx = client->getServices(true);

  if (sx->empty()) {
    client->disconnect();
    restart("No services found, will retry");
    return;
  }

  for (auto &service: *sx) {
    if (service->getUUID().equals(hidService)) {
      Log.noticeln("Discovering characteristics ...");
      auto cx = service->getCharacteristics(true);

      for (auto &characteristic: *cx) {
        if (characteristic->getUUID().equals(reportUUID)) {
          if (characteristic->canNotify()) {
            Log.noticeln("Found notification characteristic");

            Log.noticeln("Subscribing to notifications");
            characteristic->subscribe(true, onNotification);

            auto descriptor = characteristic->getDescriptor(cccdUUID);
            if (!descriptor) {
              client->disconnect();
              restart("[BUG] Descriptor not found");
              return;
            }

            Log.noticeln("Enabling notifications ...");
            descriptor->writeValue(ON, sizeof(ON), true);

            Log.noticeln("Ready to receive notifications from buttons!");

            return;
          } else {
            Log.noticeln("[BUG] Could not subscribe");
          }
        } else {
          Log.noticeln("Wrong UUID for char");
        }
      }
    } else {
      Log.noticeln("Invalid service, skip to next");
    }
  }

  Log.noticeln("Finished connecting to service?");
}

void handleClickEvent() {
  if (activeButton) {
    activeButton->tick(activeState);
  }

  /* Toggle OTA mode */
  if (activeState && !keyboard.isConnected()) {
    EEPROM.write(0, otaStatus ? 0 : 1);
    Log.noticeln("Toggle OTA");
    delay(1000);
    ESP.restart();
  }
}

void setup() {
  setupSerial();
  Log.noticeln("\nStarting ESP32 ...");

  otaStatus = EEPROM.read(0);
  if (otaStatus) {
    Log.noticeln("Enable OTA");
    setupOTA();
    return;
  }

  setupButtons();
  setupKeyboard();
  delay(1000);
  scanForDevice();
  setupClient();
  Log.noticeln("COmplete setup!");
}

void loop() {
  if (otaStatus && millis() < TEN_MINUTES) {
    handleOTA();
  } else if (otaStatus) {
    Log.noticeln("OTA mode enabled for more 10 minutes, will restart");
    EEPROM.write(0, 0);
    delay(2000);
    ESP.restart();
  } else {
    handleClickEvent();
  }

  delay(10);
};
