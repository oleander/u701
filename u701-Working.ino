#include "keyboard.h"
#include "print.h"
#include "settings.h"
#include <Arduino.h>
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <OneButton.h>

const std::map<uint8_t, const char *> hci_error_codes = {
    {0x00, "HCI_SUCCESS"},
    {0x02, "HCI_ERROR_CODE_UNKNOWN_CONN_ID"},
    {0x03, "HCI_ERROR_CODE_HW_FAILURE"},
    {0x04, "HCI_ERROR_CODE_PAGE_TIMEOUT"},
    {0x06, "HCI_ERROR_CODE_PIN_KEY_MISSING"},
    {0x07, "HCI_ERROR_CODE_MEM_CAP_EXCEEDED"},
    {0x08, "HCI_ERROR_CODE_CONN_TIMEOUT"},
    {0x09, "HCI_ERROR_CODE_CONN_LIMIT_EXCEEDED"},
    {0x12, "HCI_ERROR_CODE_INVALID_HCI_CMD_PARAMS"},
    {0x14, "HCI_ERROR_CODE_REMOTE_DEVICE_TERM_CONN_LOW_RESOURCES"},
    {0x26, "HCI_ERROR_CODE_LINK_KEY_CAN_NOT_BE_CHANGED"},
    {0x28, "HCI_ERROR_CODE_INSTANT_PASSED"},
    {0x2e, "HCI_ERROR_CODE_CHAN_ASSESSMENT_NOT_SUPPORTED"},
};

static BLEUUID hidService("1812");
static BLEUUID reportUUID("2A4D");
static BLEUUID cccdUUID("2902");

static uint8_t ON[] = {0x1, 0x0};
bool activeState    = false;

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

  PRINTLN("Got notification from ID: " + String(currentID));

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
  PRINTLN("Restarting ESP32 ...");
  PRINTLN(reason);
  delay(1000);
  ESP.restart();
}

class ClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *client) { PRINTLN("Connected to device!"); }

  void onDisconnect(BLEClient *client) {
    restart("Disconnected from device, restarting ESP32 ...");
  }
};

BLEAdvertisedDevice *device;

class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertised) {
    PRINT(".");

    if (!advertised.isAdvertisingService(hidService)) return;

    auto addr = advertised.getAddress().toString();
    if (strcmp(addr.c_str(), DEVICE_MAC) != 0) return;

    auto name = advertised.getName();
    auto rssi = advertised.getRSSI();
    auto manu = advertised.getManufacturerData();
    auto serv = advertised.getServiceData();

    PRINTF("Found device: %s (%s) RSSI=%d, manu=%d, serv=%d\n", name.c_str(), addr.c_str(), rssi,
           manu.size(), serv.size());

    PRINTLN("Connecting to advertised device ...");
    device = new BLEAdvertisedDevice(advertised);

    PRINTLN("Stopping scan ...");
    advertised.getScan()->stop();
  }
};

void scanForDevice() {
  auto scan = BLEDevice::getScan();

  scan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  scan->setInterval(SCAN_INTERVAL);
  scan->setWindow(SCAN_WINDOW);
  scan->setActiveScan(true);
  scan->start(0, false);
  scan->clearResults();
}

void setupKeyboard() {
  PRINTLN("Enable Keyboard");
  keyboard.begin();
  keyboard.setDelay(12);
}

void setupSerial() {
#if defined(INFO)
  Serial.begin(SERIAL_BAUD_RATE);
#endif
}

void setupClient() {
  auto client = BLEDevice::createClient();

  client->setClientCallbacks(new ClientCallback());
  client->connect(device);

  PRINTLN("Discovering services ...");
  auto service = client->getService(hidService);
  if (!service) {
    restart("[BUG] Service not found");
    return;
  }

  PRINTLN("Discovering characteristics ...");
  auto characteristic = service->getCharacteristic(reportUUID);
  if (!characteristic->canNotify()) {
    restart("[BUG] Characteristic cannot notify");
    return;
  }

  PRINTLN("Subscribing to notifications");
  characteristic->registerForNotify(onNotification);

  delay(50);

  auto descriptor = characteristic->getDescriptor(cccdUUID);
  if (!descriptor) {
    restart("[BUG] Descriptor not found");
    return;
  }

  PRINTLN("Enabling notifications ...");
  descriptor->writeValue(ON, sizeof(ON), true);
}

void handleClickEvent() {
  if (activeButton) {
    activeButton->tick(activeState);
  }
}

void setup() {
  setupSerial();
  PRINTLN("\nStarting ESP32 ...");

  setupKeyboard();
  setupButtons();
  scanForDevice();
  setupClient();
  setupOTA();
}

void loop() {
  handleOTA();
  handleClickEvent();
};
