// #include <BleKeyboard.h>
#include "ffi.h"
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
#include <vector>

#define SCAN_INTERVAL 500 // in ms
#define SCAN_WINDOW   450 // in ms

#define SERIAL_BAUD_RATE 115200
#define DEVICE_BATTERY   100

#define DEVICE_MANUFACTURER "HVA"
// #define DEVICE_NAME         "u701"
// #define DEVICE_NAME "key"
#define DEVICE_NAME "Terrain Comman"

// A8:42:E3:CD:FB:C6, f7:97:ac:1f:f8:c0
// a8:42:e3:cd:fb:c6,
NimBLEAddress ServerAddress(0xA842E3CD0C6, BLE_ADDR_RANDOM);
// NimBLEAddress ServerAddress = 0xF797AC1FF8C0; // REAL

#include <stdarg.h>

void restart(const char *format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  Serial.println(buffer);
  Serial.println("Will restart the ESP in 2 seconds");
  delay(2000);
  ESP.restart();
}

BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);
void scanEndedCB(NimBLEScanResults results);
static NimBLEUUID serviceUUID("1812");
static NimBLEUUID charUUID("2a4d");

class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *pClient) {
    Serial.println("Connected, will update conn params");
    pClient->updateConnParams(120, 120, 0, 1);
  };

  void onDisconnect(NimBLEClient *pClient) {
    restart("Terrain Command disconnected");
  };

  /** Called when the peripheral requests a change to the connection parameters.
        Return true to accept and apply them or false to reject and keep
        the currently used parameters. Default will return true.
  */
  bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params) {
    Serial.println("Connection parameter update request: ");

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
    Serial.println("Encrypt connection failed - disconnecting");
    NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
  };
};

static ClientCallbacks clientCB;
static NimBLEAdvertisedDevice *advDevice;

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    Serial.print(".");
    // Print the name
    printf("Advertised Device: %s\n", advertisedDevice->toString().c_str());

    // if (!advertisedDevice->isAdvertisingService(serviceUUID)) {
    //   printf("Found device: %s\n", advertisedDevice->toString().c_str());
    //   return;
    // }

    // if (!advertisedDevice->getAddress().equals(ServerAddress)) {
    //   printf("Found device: %s\n", advertisedDevice->getAddress());
    //   return;
    // }

    if (advertisedDevice->getName() != DEVICE_NAME) {
      // printf("Found device: %s\n", advertisedDevice->getName().c_str());
      return;
    }

    Serial.println("Found Our Service, stopping scan");
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
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println("Starting NimBLE Client");

  NimBLEDevice::init("");
  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  Serial.println("Starting BLE scan");

  auto pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setInterval(SCAN_INTERVAL);
  pScan->setWindow(SCAN_WINDOW);
  pScan->setActiveScan(true);
  pScan->start(50200, false);

  Serial.println("Starting keyboard");
  keyboard.begin();

  if (!advDevice) {
    restart("No advertised device found while scanning");
  }

  auto pClient = NimBLEDevice::createClient();
  pClient->setClientCallbacks(&clientCB, false);
  pClient->setConnectionParams(12, 12, 0, 51);
  pClient->setConnectTimeout(5);

  if (!pClient->connect(advDevice)) {
    restart("Could not connect to the Terrain Command");
  } else if (!pClient->isConnected()) {
    restart("Could not connect to the Terrain Command");
  }

  Serial.println("Connected to server");

  auto pSvc = pClient->getService(serviceUUID);
  if (!pSvc) {
    Serial.println("Failed to find our service UUID");
    Serial.println("Will disconnect the device");
    pClient->disconnect();
    restart("Device has been manually disconnected");
  }

  auto pChrs = pSvc->getCharacteristics(true);
  if (!pChrs) {
    Serial.println("Failed to find our characteristic UUID");
    Serial.println("Will disconnect the device");
    pClient->disconnect();
    restart("Device has been manually disconnected");
  }

  for (int i = 0; i < pChrs->size(); i++) {
    if (!pChrs->at(i)->canNotify()) {
      Serial.println("Characteristic cannot notify");
      continue;
    }

    if (!pChrs->at(i)->getUUID().equals(charUUID)) {
      Serial.println("Characteristic UUID does not match");
      continue;
    }

    if (!pChrs->at(i)->subscribe(true, onEvent, false)) {
      Serial.println("Failed to subscribe to characteristic");
      Serial.println("Will disconnect the device");
      pClient->disconnect();
      restart("Device has been manually disconnected");
    }

    Serial.println("Successfully subscribed to characteristic");
    return;
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
