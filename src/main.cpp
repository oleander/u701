// #include <BleKeyboard.h>
#include "ffi.h"
#include <Arduino.h>
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
#include <stdarg.h>
#include <stdlib.h>
#include <vector>

#define SCAN_DURATION 5 * 60 // in seconds
#define SCAN_INTERVAL 500    // in ms
#define SCAN_WINDOW   450    // in ms

#define SERIAL_BAUD_RATE 115200
#define DEVICE_BATTERY   100

#define CLIENT_NAME         "Terrain Comman"
#define DEVICE_NAME         "u701"
#define DEVICE_MANUFACTURER "HVA"

// A8:42:E3:CD:FB:C6, f7:97:ac:1f:f8:c0
// NimBLEAddress ServerAddress(0xA842E3CD0C6, BLE_ADDR_RANDOM); // TEST
NimBLEAddress serverAddress(0xF797AC1FF8C0, BLE_ADDR_RANDOM); // REAL
static NimBLEUUID serviceUUID("1812");
static NimBLEUUID charUUID("2a4d");

/**
 * Restarts the ESP device with a formatted message. Uses dynamic memory allocation
 * to handle potentially large messages.
 *
 * @param format A format string for the message.
 * @param ... Variable arguments for the format string.
 */
void restart(const char *format, ...) {
  // Start with a reasonable buffer size
  size_t bufferSize = 256;
  char *buffer      = (char *) malloc(bufferSize);

  if (buffer == NULL) {
    Serial.println("Memory allocation failed. Restarting...");
    ESP.restart();
  }

  // Initialize the variable argument list
  va_list args;
  va_start(args, format);

  // Try to format the message into the buffer
  int needed = vsnprintf(buffer, bufferSize, format, args);

  // Check if the buffer was large enough
  if (needed >= bufferSize) {
    // Reallocate with the correct size
    bufferSize      = needed + 1; // +1 for null terminator
    char *newBuffer = (char *) realloc(buffer, bufferSize);

    if (newBuffer == NULL) {
      free(buffer);
      Serial.println("Memory reallocation failed. Restarting...");
      ESP.restart();
    }

    buffer = newBuffer;

    // Format the message again
    va_end(args);
    va_start(args, format);
    vsnprintf(buffer, bufferSize, format, args);
  }

  // Clean up the variable argument list
  va_end(args);

  // Print the formatted message to the Serial interface
  Serial.println(buffer);

  // Notify about the impending restart
  Serial.println("Will restart the ESP in 2 seconds");

  // Wait for 2 seconds
  delay(2000);

  // Free the allocated memory
  free(buffer);

  // Restart the ESP device
  ESP.restart();
}

BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);
static NimBLEClient *pClient;

/* Event received from the Terrain Command */
static void onEvent(BLERemoteCharacteristic *_, uint8_t *data, size_t length, bool isNotify) {
  if (!isNotify) {
    return;
  }

  c_on_event(data, length);
}

// Terrain Command BLE buttons
class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *pClient) {
    Serial.println("Connected, will optimize conn params");
    pClient->updateConnParams(120, 120, 0, 1);

    Serial.println("Re-broadcasting keyboard");
    keyboard.broadcast();
  };

  void onDisconnect(NimBLEClient *pClient) {
    restart("Terrain Command disconnected");
  };

  /** Called when the peripheral requests a change to the connection parameters.
        Return true to accept and apply them or false to reject and keep
        the currently used parameters. Default will return true.
  */
  bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params) {
    Serial.println("Connection parameters update request received");
    printf("Requested connection params: interval: %d, latency: %d, supervision timeout: %d\n",
           params->itvl_min,
           params->latency,
           params->supervision_timeout);

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
    Serial.println("Connection with Terrain Command established");

    if (!desc->sec_state.encrypted) {
      restart("Encrypt connection failed: %s", desc);
    }

    Serial.println("Fetching service ...");
    auto pSvc = pClient->getService(serviceUUID);
    if (!pSvc) {
      Serial.println("Failed to find our service UUID");
      Serial.println("Will disconnect the device");
      pClient->disconnect();
      restart("Device has been manually disconnected");
    }

    Serial.println("Fetching all characteristics ...");
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

      if (!pChrs->at(i)->subscribe(true, onEvent, true)) {
        Serial.println("Failed to subscribe to characteristic");
        Serial.println("Will disconnect the device");
        pClient->disconnect();
        restart("Device has been manually disconnected");
      }

      Serial.println("Successfully subscribed to characteristic");
      return;
    }

    restart("[BUG] Didn't find any matching characteristics");
  };
};

ClientCallbacks clientCallbacks;

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    Serial.print(".");

    if (!advertisedDevice->isAdvertisingService(serviceUUID)) {
      return;
    } else if (advertisedDevice->getName() != CLIENT_NAME) {
      return;
    } else if (advertisedDevice->getAddress() != serverAddress) {
      return;
    } else {
      Serial.println("\nFound the Terrain Command");
    }

    auto addr = advertisedDevice->getAddress();
    pClient   = NimBLEDevice::createClient(addr);
    advertisedDevice->getScan()->stop();
  };
};

extern "C" void init_arduino() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println("Starting ESP32 BLE Proxy");

  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  NimBLEDevice::setPower(ESP_PWR_LVL_P3);
  NimBLEDevice::init(DEVICE_NAME);

  Serial.println("Starting BLE scan");

  Serial.println("Starting keyboard");
  keyboard.begin();

  // Serial.println("Starting keyboard");
  // while (!keyboard.isConnected()) {
  //   Serial.print(".");
  //   delay(100);
  // }

  auto pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setInterval(SCAN_INTERVAL);
  pScan->setWindow(SCAN_WINDOW);
  pScan->setActiveScan(true);
  pScan->setMaxResults(0);
  pScan->start(SCAN_DURATION, false);

  if (!pClient) {
    restart("The Terrain Command was not found");
  }

  pClient->setClientCallbacks(&clientCallbacks, false);
  pClient->setConnectionParams(12, 12, 0, 51);
  pClient->setConnectTimeout(10);

  if (!pClient->connect()) {
    restart("Could not connect to the Terrain Command");
  } else if (!pClient->isConnected()) {
    restart("Could not connect to the Terrain Command");
  } else {
    Serial.println("Successfully connected to the Terrain Command");
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

// static int ble_client_gap_event(struct ble_gap_event *event, void *arg) {
//   ble_client *client = (ble_client *) arg;
//   ESP_LOGD(TAG, "gap event: %d", event->type);
//   switch (event->type) {
//   case BLE_GAP_EVENT_CONNECT: {
//     int status = event->connect.status;
//     if (status == 0) {
//       ESP_LOGI(TAG, "connection established");
//       client->conn_handle = event->connect.conn_handle;
//     } else {
//       ESP_LOGE(TAG, "connection failed. ble code: %d", status);
//       client->semaphore_result = ble_client_convert_ble_code(status);
//       client->connected        = false;
//       xSemaphoreGive(client->semaphore);
//     }
//     break;
//   }
//     // notify connected only after MTU negotiation completes
//   case BLE_GAP_EVENT_MTU: {
//     ESP_LOGI(TAG, "MTU negotiated");
//     client->semaphore_result = ESP_OK;
//     client->connected        = true;
//     xSemaphoreGive(client->semaphore);
//     break;
//   }
//   default:
//     break;
//   }
//   return 0;
// }
