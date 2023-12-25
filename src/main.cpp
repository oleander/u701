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

BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);

// A8:42:E3:CD:FB:C6, f7:97:ac:1f:f8:c0
NimBLEAddress ServerAddress = 0xA842E3CD0C6;

void scanEndedCB(NimBLEScanResults results);

// UUID HID
static NimBLEUUID serviceUUID("1812");
static NimBLEUUID charUUID("2a4d");

static NimBLEAdvertisedDevice *advDevice;

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
    if (!desc->sec_state.encrypted) {
      printf("Encrypt connection failed - disconnecting");
      /** Find the client with the connection handle provided in desc */
      NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
      return;
    }
  };
};

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {

  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    printf("Advertised Device: %s \n", advertisedDevice->toString().c_str());

    if (!advertisedDevice->isAdvertisingService(serviceUUID)) {
      return;
    }

    if (!advertisedDevice->getAddress().equals(ServerAddress)) {
      return;
    }

    printf("Found Our Service!");
    /** stop scan before connecting */
    advDevice->getScan()->stop();
    /** Save the device reference in a global for the client to use*/
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

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results) {
  printf("Scan Ended\n");
}

/** Create a single global instance of the callback class to be used by all clients */
static ClientCallbacks clientCB;

/** Handles the provisioning of clients and connects / interfaces with the server */
bool connectToServer() {
  NimBLEClient *pClient = nullptr;

  /** Check if we have a client we should reuse first **/
  if (NimBLEDevice::getClientListSize()) {
    /** Special case when we already know this device, we send false as the
        second argument in connect() to prevent refreshing the service database.
        This saves considerable time and power.
    */
    pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
    if (pClient) {
      if (!pClient->connect(advDevice, false)) {
        printf("Reconnect failed\n");
        return false;
      }
      printf("Reconnected client\n");
    }
    /** We don't already have a client that knows this device,
        we will check for a client that is disconnected that we can use.
    */
    else {
      pClient = NimBLEDevice::getDisconnectedClient();
    }
  }

  /** No client to reuse? Create a new one. */
  if (!pClient) {
    if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
      printf("Failed to connect, restarting ESP (0)");
      return false;
    }

    pClient = NimBLEDevice::createClient();

    printf("New client created\n");
    pClient->setClientCallbacks(&clientCB, false);
    /** Set initial connection parameters: These settings are 15ms interval, 0 latency, 120ms timout.
        These settings are safe for 3 clients to connect reliably, can go faster if you have less
        connections. Timeout should be a multiple of the interval, minimum is 100ms.
        Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout
    */
    pClient->setConnectionParams(12, 12, 0, 51);
    /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
    pClient->setConnectTimeout(5);

    if (!pClient->connect(advDevice)) {
      printf("Failed to connect, restarting ESP (1)");
      ESP.restart();
      return false;
    }
  }

  if (!pClient->isConnected()) {
    if (!pClient->connect(advDevice)) {
      printf("Failed to connect, restarting ESP (2)");
      ESP.restart();
      return false;
    }
  }

  printf("Connected to: %s\n", pClient->getPeerAddress().toString().c_str());

  /** Now we can read/write/subscribe the charateristics of the services we are interested in */
  NimBLERemoteService *pSvc = nullptr;
  //  NimBLERemoteCharacteristic *pChr = nullptr;
  std::vector<NimBLERemoteCharacteristic *> *pChrs = nullptr;

  NimBLERemoteDescriptor *pDsc = nullptr;

  pSvc = pClient->getService(serviceUUID);
  if (pSvc) { /** make sure it's not null */
    pChrs = pSvc->getCharacteristics(true);
  }

  if (pChrs) { /** make sure it's not null */

    for (int i = 0; i < pChrs->size(); i++) {

      if (pChrs->at(i)->canNotify()) {
        /** Must send a callback to subscribe, if nullptr it will unsubscribe */
        if (!pChrs->at(i)->registerForNotify(onEvent)) {
          /** Disconnect if subscribe failed */
          pClient->disconnect();
          return false;
        }
      }
    }
  }

  else {
    printf("ERROR: Failed to find our characteristic UUID\n");
  }

  printf("Done with this device!\n\n");
  return true;
}

extern "C" void init_arduino() {
  printf("Starting NimBLE Client\n");

  // NimBLEDevice::init("");

  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  /** Optional: set the transmit power, default is 3db */
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
  /** create new scan */
  NimBLEScan *pScan = NimBLEDevice::getScan();

  /** create a callback that gets called when advertisers are found */
  printf("Scan started\n");
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setActiveScan(true);
  pScan->setInterval(97);
  pScan->setWindow(37);
  pScan->start(0);

  printf("Starting keyboard\n");
  keyboard.begin();

  printf("Starting BLE work!\n");
  if (connectToServer()) {
    printf("We are now connected to the BLE Server.\n");
  } else {
    printf("We have failed to connect to the server; there is nothin more we will do.\n");
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
