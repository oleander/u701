#include "keyboard.h"
#include "print.h"
#include "watchdog.h"
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <OneButton.h>

#define SERIAL_BAUD_RATE 115200
#define SCAN_INTERVAL    2000
#define SCAN_WINDOW      1500
#define SCAN_DURATION    5

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
BLEScan *scan     = BLEDevice::getScan();

const char *getErrorMessage(uint8_t error_code) {
  auto it = hci_error_codes.find(error_code);
  if (it != hci_error_codes.end()) {
    return it->second;
  }
  return nullptr;
}

enum State { SCAN_DEVICE, CONNECT_TO_DEVICE, DEVICE_CONNECTED, INITIALIZE, FINISHED, DISCONNECTED };
static State state = SCAN_DEVICE;

bool connectToServer() {
  if (device == nullptr) {
    PRINTLN("[Client] [BUG] Address is null");
    return false;
  }

  PRINTLN("[Client] Discovering services ...");
  auto service = client->getService(hidService);
  if (!service) {
    PRINTLN("[Client] [BUG] Service not found");
    return false;
  }

  auto characteristic = service->getCharacteristic(reportUUID);
  if (!characteristic->canNotify()) {
    PRINTLN("[Client] [BUG] Characteristic cannot notify");
    return false;
  }

  PRINTLN("[Client] Subscribing to notifications");
  characteristic->registerForNotify(onNotification);

  delay(50);

  auto descriptor = characteristic->getDescriptor(cccdUUID);
  if (!descriptor) {
    PRINTLN("[Client] [BUG] Descriptor not found");
    return false;
  }

  PRINTLN("[Client] Enabling notifications");
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

  if (currentID == 0x0000) { // Button was released
    activeState = false;
  } else if (!activeState) { // Button was pressed, and another button is not already pressed
    activeState  = true;
    activeButton = buttons.at(currentID);
  }
}

class ClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *client) {
    PRINTLN("[Client] Connected to BLE buttons");
    state = DEVICE_CONNECTED;
  }

  void onDisconnect(BLEClient *client) {
    PRINTLN("[Client] Disconnected from BLE buttons");
    state = DISCONNECTED;
  }
};

class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertised) {
    PRINT(".");

    if (!advertised.isAdvertisingService(hidService)) return;

    auto addr = advertised.getAddress().toString();
    if (strcmp(addr.c_str(), "f7:97:ac:1f:f8:c0") != 0) return;

    auto name = advertised.getName();
    auto rssi = advertised.getRSSI();
    auto manu = advertised.getManufacturerData();
    auto serv = advertised.getServiceData();

    PRINTF("[Client] Found device: %s (%s) RSSI=%d, manu=%d, serv=%d\n", name.c_str(), addr.c_str(),
           rssi, manu.size(), serv.size());

    PRINTLN("[Client] Found controller unit");
    device = new BLEAdvertisedDevice(advertised);

    PRINTLN("[Client] Stopping scan");
    advertised.getScan()->stop();

    state = CONNECT_TO_DEVICE;

    return;
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

  PRINTLN("[Client] Starting...");

  scan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  scan->setInterval(SCAN_INTERVAL);
  scan->setWindow(SCAN_WINDOW);
  scan->setActiveScan(true);

  client->setClientCallbacks(new ClientCallback());

  setupButtons();
  setupWatchdog();
  PRINTLN("");

#if defined(DEBUG)
  BLEDevice::setCustomGattcHandler(gattcEventHandler);
  BLEDevice::setCustomGattsHandler(my_gatts_event_handler);
  BLEDevice::setCustomGapHandler(my_gap_event_handler);
#endif
}

void loop() {
  switch (state) {
  case SCAN_DEVICE:
    PRINTLN("\n[Client] Scanning for buttons");
    scan->start(10, false);
    scan->clearResults();
    break;
  case CONNECT_TO_DEVICE:
    PRINTLN("[Client] Connecting to buttons");
    client->connect(device);
    break;
  case DEVICE_CONNECTED:
    if (connectToServer()) {
      PRINTLN("Everything good, connected!");
      state = FINISHED;
    } else {
      state = SCAN_DEVICE;
    }
    break;
  case DISCONNECTED:
    PRINTLN("Will restart as device has disconnected");
    ESP.restart();
  case FINISHED:
    if (activeButton) {
      activeButton->tick(activeState);
    }

    handleWatchdog();
    break;
  }

};
