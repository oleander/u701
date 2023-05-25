#include "keyboard.h"
#include "print.h"
#include "settings.h"
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <OneButton.h>

OneButton *activeButton;
bool activeState = false;

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

static BLEAdvertisedDevice *device = nullptr;

BLEClient *client = BLEDevice::createClient();

const char *getErrorMessage(uint8_t error_code) {
  auto it = hci_error_codes.find(error_code);
  if (it != hci_error_codes.end()) {
    return it->second;
  }
  return nullptr;
}

bool connectToServer() {
  if (device == nullptr) {
    PRINTLN("[BUG] Address is null");
    return false;
  }

  PRINTLN("Discovering services ...");
  auto service = client->getService(hidService);
  if (!service) {
    PRINTLN("[BUG] Service not found");
    return false;
  }

  auto characteristic = service->getCharacteristic(reportUUID);
  if (!characteristic->canNotify()) {
    PRINTLN("[BUG] Characteristic cannot notify");
    return false;
  }

  PRINTLN("Subscribing to notifications");
  characteristic->registerForNotify(onNotification);

  delay(50);

  auto descriptor = characteristic->getDescriptor(cccdUUID);
  if (!descriptor) {
    PRINTLN("[BUG] Descriptor not found");
    return false;
  }

  PRINTLN("Enabling notifications");
  descriptor->writeValue(ON, sizeof(ON), true);

  return true;
}

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

class ClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *client) {
    PRINTLN("Connected to BLE buttons");
    state = CONNECTED;
  }

  void onDisconnect(BLEClient *client) {
    PRINTLN("Disconnected from BLE buttons");
    state = DISCONNECTED;
  }
};

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

    PRINTLN("Found controller unit");
    device = new BLEAdvertisedDevice(advertised);

    PRINTLN("Stopping scan");
    advertised.getScan()->stop();
  }
};

// Callback function for GATT client
void gattcEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                       esp_ble_gattc_cb_param_t *param) {
  const char *error_msg = getErrorMessage(event);
  if (error_msg) {
    PRINTF("EVT: %s\n", error_msg);
  } else {
    PRINTF("EVT %x\n", event);
  }
}

void my_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  const char *error_msg = getErrorMessage(event);
  if (error_msg) {
    PRINTF("GAP EVT: %s, PARAMS: %x\n", error_msg, param->scan_rst.search_evt);
  } else {
    PRINTF("GAP EVT %x, PARAMS: %x\n", event, param->scan_rst.search_evt);
  }
}

void my_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                            esp_ble_gatts_cb_param_t *param) {
  const char *error_msg = getErrorMessage(event);
  if (error_msg) {
    PRINTF("GATTS EVT: %s\n", error_msg);
  } else {
    PRINTF("GATTS EVT %x\n", event);
  }
}

void setup() {
#if defined(INFO)
  Serial.begin(SERIAL_BAUD_RATE);
#endif

  PRINTLN("\nStarting ESP32 ...");

  PRINTLN("Enable Keyboard");
  keyboard.begin();
  keyboard.setDelay(12);

  PRINTLN("Starting...");

  auto scan = BLEDevice::getScan();

  scan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  scan->setInterval(SCAN_INTERVAL);
  scan->setWindow(SCAN_WINDOW);
  scan->setActiveScan(true);

  client->setClientCallbacks(new ClientCallback());

  setupButtons();

  PRINTLN("");

#if defined(DEBUG)
  BLEDevice::setCustomGattcHandler(gattcEventHandler);
  BLEDevice::setCustomGattsHandler(my_gatts_event_handler);
  BLEDevice::setCustomGapHandler(my_gap_event_handler);
#endif

  scan->start(0, false);
  scan->clearResults();

  if (device == nullptr) {
    PRINTLN("[BUG] Device is null, will restart");
    delay(1000); // Give the serial monitor some time
    ESP.restart();
  }

  PRINTLN("Connecting to Terrain Command buttons");
  client->connect(device);
}

void loop() {
  switch (state) {
  case SUBSCRIBED:
    if (activeButton) {
      activeButton->tick(activeState);
    }
    break;
  case CONNECTED:
    if (!connectToServer()) {
      PRINTLN("Failed to connect to server, will restart");
      delay(1000); // Give the serial monitor some time
      ESP.restart();
    }

    state = SUBSCRIBED;
    PRINTLN("Subscribed to notifications");
    break;
  case DISCONNECTED:
    PRINTLN("Device disconnected, will restart");
    delay(1000); // Give the serial monitor some time
    ESP.restart();
    break;
  case CONNECTING:
    PRINT(".");
    delay(100);
    break;
  }
};
