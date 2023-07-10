#include "keyboard.h"
#include "print.h"
#include "settings.h"
#include <Arduino.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
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

static NimBLEUUID hidService("1812");
static NimBLEUUID reportUUID("2A4D");
static NimBLEUUID cccdUUID("2902");

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

// const char *getErrorMessage(uint8_t error_code) {
//   auto it = hci_error_codes.find(error_code);
//   if (it != hci_error_codes.end()) {
//     return it->second;
//   }
//   return nullptr;
// }

static void onNotification(BLERemoteCharacteristic *characteristic, uint8_t *data, size_t length,
                           bool isNotify) {
  if (!isNotify) return;
  if (length != 4) return;

  auto currentID = dataToInt(data, length);

  PRINTF("Got notification from ID 0x%x\n", currentID);

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
  delay(5000);
  ESP.restart();
}

class ClientCallback : public NimBLEClientCallbacks {
  void onConnect(BLEServer *pServer) { PRINTLN("Connected to device!"); }

  void onDisconnect(NimBLEClient *client) {
    restart("Disconnected from device, restarting ESP32 ...");
  }
};

NimBLEAdvertisedDevice *device;

class Callbacks : public NimBLEAdvertisedDeviceCallbacks {
public:
  void onResult(NimBLEAdvertisedDevice *advertised) {
    PRINT(".");

    if (!advertised->isAdvertisingService(hidService)) return;
    
    // if (!advertised->isConnectable()) return;

    auto addr = advertised->getAddress().toString();
    if (strcmp(addr.c_str(), DEVICE_MAC) != 0) return;

    auto name = advertised->getName();
    auto rssi = advertised->getRSSI();
    auto manu = advertised->getManufacturerData();
    auto serv = advertised->getServiceData();

    PRINTF("Found device: %s (%s) RSSI=%d, manu=%d, serv=%d\n", name.c_str(), addr.c_str(), rssi,
           manu.size(), serv.size());

    PRINTLN("Connecting to advertised device ...");
    device = advertised;

    PRINTLN("Stopping scan ...");
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
  PRINTLN("Enable Keyboard");
  keyboard.begin();
  keyboard.setDelay(12);
  // while (!keyboard.isConnected()) {
  //   PRINT(".");
  //   delay(300);
  // }

  // PRINT("\n");
}

void setupSerial() {
#if defined(INFO)
  Serial.begin(SERIAL_BAUD_RATE);
#endif
}

NimBLEClient* client;

void setupClient() {
  client = NimBLEDevice::createClient();

  client->setClientCallbacks(new ClientCallback());
  client->connect(device);

  // delay(150);

  // PRINTLN("Waiting to be connected to buttons");
  // while (!client->isConnected()) {
  //   sleep(500);
  //   PRINT(".");
  // }

  // PRINTLN("Phone buttons");
  // return;
  PRINTLN("Discovering services ...");
  // auto service = client->getService(hidService);
  // delay(5000);
  auto sx = client->getServices(true);

  if (sx->empty()) {
    client->disconnect();
    restart("No services found, will retry");
    return;
  }

  for (auto &service: *sx) {
    if (service->getUUID().equals(hidService)) {
      PRINTLN("Discovering characteristics ...");
      // auto characteristic;
      auto cx = service->getCharacteristics(true);

      for (auto &characteristic: *cx) {
        if (characteristic->getUUID().equals(reportUUID)) {
          if (characteristic->canNotify()) {
            PRINTLN("Found notification characteristic");
            // delay(150);

            PRINTLN("Subscribing to notifications");
            characteristic->registerForNotify(onNotification);

            // delay(150);

            auto descriptor = characteristic->getDescriptor(cccdUUID);
            if (!descriptor) {
              client->disconnect();
              restart("[BUG] Descriptor not found");
              return;
            }

            PRINTLN("Enabling notifications ...");
            descriptor->writeValue(ON, sizeof(ON), true);

            PRINTLN("Ready to receive notifications from buttons!");

            return;
          } else {
            PRINTLN("[BUG] Could not subscribe");
          }
        } else {
          PRINTLN("Wrong UUID for char");
        }
      }
    } else {
      PRINTLN("Invalid service, skip to next");
    }
  }

  PRINTLN("Finished connecting to service?");
}

void handleClickEvent() {
  if (activeButton) {
    activeButton->tick(activeState);
  }
}

// void gattcEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
//                        esp_ble_gattc_cb_param_t *param) {
//   const char *error_msg = getErrorMessage(event);
//   if (error_msg) {
//     PRINTF("EVT: %s\n", error_msg);
//   } else {
//     PRINTF("EVT %x\n", event);
//   }
// }

// void my_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
//   const char *error_msg = getErrorMessage(event);
//   if (error_msg) {
//     PRINTF("GAP EVT: %s, PARAMS: %x\n", error_msg, param->scan_rst.search_evt);
//   } else {
//     PRINTF("GAP EVT %x, PARAMS: %x\n", event, param->scan_rst.search_evt);
//   }
// }

// void my_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
//                             esp_ble_gatts_cb_param_t *param) {
//   const char *error_msg = getErrorMessage(event);
//   if (error_msg) {
//     PRINTF("GATTS EVT: %s\n", error_msg);
//   } else {
//     PRINTF("GATTS EVT %x\n", event);
//   }
// }

void setup() {
  setupSerial();
  PRINTLN("\nStarting ESP32 ...");

  // BLEDevice::setCustomGattcHandler(gattcEventHandler);
  // BLEDevice::setCustomGattsHandler(my_gatts_event_handler);
  // BLEDevice::setCustomGapHandler(my_gap_event_handler);

  setupButtons();
  setupKeyboard();
  delay(1000);
  scanForDevice();
  setupClient();
  PRINTLN("COmplete setup!");
}

bool otaLoaded = false;
void loop() { handleClickEvent(); };
