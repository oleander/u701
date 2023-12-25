// #include <BleKeyboard.h>
#include "ffi.h"
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
#include <vector>

#define SCAN_INTERVAL       500 // in ms
#define SCAN_WINDOW         450 // in ms
#define DEVICE_NAME         "u701"
#define SERIAL_BAUD_RATE    115200
#define DEVICE_MANUFACTURER "u701"
#define DEVICE_BATTERY      100
#define ServerName          "u701"

// A8:42:E3:CD:FB:C6, f7:97:ac:1f:f8:c0
NimBLEAddress ServerAddress = 0xA842E3CD0C6; // TEST
// NimBLEAddress ServerAddress = 0xF797AC1FF8C0; // REAL

BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);
void scanEndedCB(NimBLEScanResults results);
static NimBLEUUID serviceUUID("1812");
static NimBLEUUID charUUID("2a4d");

class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *pClient) {
    printf("Connected, will update conn params\n");
    pClient->updateConnParams(120, 120, 0, 1);
  };

  void onDisconnect(NimBLEClient *pClient) {
    printf("Disconnected\n");
    ESP.restart();
  };

  /** Called when the peripheral requests a change to the connection parameters.
        Return true to accept and apply them or false to reject and keep
        the currently used parameters. Default will return true.
  */
  bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params) {
    printf("Connection parameter update request: ");

    if (params->itvl_min < 24) { /** 1.25ms units */
      return false;
    } else if (params->itvl_max > 40) { /** 1.25ms units */
      return false;
    } else if (params->latency > 2) { /** Number of intervals allowed to skip */
      return false;
    } else if (params->supervision_timeout > 100) { /** 10ms units */
      return false;
    }

    return true;
  };

  /** Pairing proces\s complete, we can check the results in ble_gap_conn_desc */
  void onAuthenticationComplete(ble_gap_conn_desc *desc) {
    if (desc->sec_state.encrypted) return;
    printf("Encrypt connection failed - disconnecting");
    NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
  };
};

static ClientCallbacks clientCB;
static NimBLEAdvertisedDevice *advDevice;

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {

    if (!advertisedDevice->isAdvertisingService(serviceUUID)) {
      printf("Not Our Service: %s\n", advertisedDevice->toString().c_str());
      return;
    }

    if (!advertisedDevice->getAddress().equals(ServerAddress)) {
      printf("Not Our Server: %s\n", advertisedDevice->toString().c_str());
      return;
    }

    printf("Found Our Service: %s\n", advertisedDevice->toString().c_str());

    advDevice->getScan()->stop();
    advDevice = advertisedDevice;
  };
};

/* Event received from the Terrain Command */
static void onEvent(BLERemoteCharacteristic *_, uint8_t *data, size_t length, bool isNotify) {
  if (!isNotify) {
    return;
  }

  c_on_event(data, length);
}

extern "C" void init_arduino() {
  printf("Starting NimBLE Client\n");

  NimBLEDevice::init("");
  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  printf("Starting BLE scan\n");

  auto pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setActiveScan(false);
  pScan->setInterval(97);
  pScan->setWindow(37);
  pScan->start(std::numeric_limits<uint32_t>::max());

  printf("Starting keyboard\n");
  keyboard.begin();

  if (!advDevice) {
    printf("No advertised device to connect to\n");
    printf("Will restart the ESP\n");
    ESP.restart();
  }

  auto pClient = NimBLEDevice::createClient();
  pClient->setClientCallbacks(&clientCB, false);
  pClient->setConnectionParams(12, 12, 0, 51);
  pClient->setConnectTimeout(5);

  if (!pClient->connect(advDevice)) {
    printf("Failed to connect, restarting ESP (1)");
    printf("Will restart the ESP\n");
    ESP.restart();
  }

  if (!pClient->isConnected()) {
    printf("Failed to connect, restarting ESP (2)");
    printf("Will restart the ESP\n");
    ESP.restart();
  }

  printf("Connected to: %s\n", pClient->getPeerAddress().toString().c_str());

  auto pSvc = pClient->getService(serviceUUID);
  if (!pSvc) {
    printf("Failed to find our service UUID: %s\n", serviceUUID.toString().c_str());
    printf("Will logout the device\n");
    pClient->disconnect();
    printf("Will restart the ESP\n");
    ESP.restart();
  }

  auto pChrs = pSvc->getCharacteristics(true);
  if (!pChrs) {
    printf("Failed to find our characteristic UUID: %s\n", charUUID.toString().c_str());
    printf("Will logout the device\n");
    pClient->disconnect();
    printf("Will restart the ESP\n");
    ESP.restart();
  }

  for (int i = 0; i < pChrs->size(); i++) {
    if (!pChrs->at(i)->canNotify()) {
      printf("Characteristic cannot notify\n");
      continue;
    }

    if (!pChrs->at(i)->getUUID().equals(charUUID)) {
      printf("Found characteristic UUID: %s\n", pChrs->at(i)->getUUID().toString().c_str());
      continue;
    }

    if (!pChrs->at(i)->registerForNotify(onEvent)) {
      printf("Failed to register for notify\n");
      continue;
    }

    if (!pChrs->at(i)->subscribe(true, onEvent, false)) {
      printf("Failed to subscribe for notify\n");
      printf("Will logout the device\n");
      pClient->disconnect();
      printf("Will restart the ESP\n");
      ESP.restart();
    }
  }
}

extern "C" void ble_keyboard_write(uint8_t c[2]) {
  if (keyboard.isConnected()) {
    keyboard.write(c);
  }
}

extern "C" void ble_keyboard_print(const uint8_t *format) {
  if (keyboard.isConnected()) {
    keyboard.print(reinterpret_cast<const char *>(format));
  }
}

extern "C" bool ble_keyboard_is_connected() {
  return keyboard.isConnected();
}
