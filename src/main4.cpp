
// // A BLE proxy
// // 1. Scans for a BLE device
// // 2. Connects to the device
// // 3. Subscribes to the device
// // 4. Forwards events to the host

// // The mapping is done elsewhere

// #include "ffi.h" // Ignore

// #include <BleKeyboard.h>
// #include <NimBLEDevice.h>
// #include <NimBLEScan.h>
// #include <NimBLEUtils.h>

// static NimBLEUUID reportUUID("2A4D");
// static NimBLEUUID hidService("1812");

// #define DEVICE_MAC "A8:42:E3:CD:FB:C6"
// // #define DEVICE_MAC          "f7:97:ac:1f:f8:c0"
// #define SCAN_INTERVAL       500 // in ms
// #define SCAN_WINDOW         450 // in ms
// #define DEVICE_NAME         "u701"
// #define SERIAL_BAUD_RATE    115200
// #define DEVICE_MANUFACTURER "u701"
// #define DEVICE_BATTERY      100

// BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);
// static auto scan             = NimBLEDevice::getScan();
// static auto buttonMacAddress = NimBLEAddress(DEVICE_MAC, 1);
// static NimBLEClient *client  = nullptr;

// /* Event received from the Terrain Command */
// static void handleButtonClick(BLERemoteCharacteristic *_, uint8_t *data, size_t length, bool isNotify) {
//   if (!isNotify) {
//     return;
//   }

//   c_on_event(data, length);
// }

// /* Terrain Command callbacks */
// class ClientCallback : public NimBLEClientCallbacks {
//   void onConnect(NimBLEClient *client) override {
//     printf("Connected to BLE device\n");
//     for (auto &service: *client->getServices(false)) {
//       for (auto &characteristic: *service->getCharacteristics(false)) {
//         if (!service->getUUID().equals(hidService)) {
//           continue;
//         } else if (!characteristic->getUUID().equals(reportUUID)) {
//           continue;
//         } else if (!characteristic->canNotify()) {
//           continue;
//         } else if (!characteristic->subscribe(true, handleButtonClick, false)) {
//           continue;
//         } else {
//           return;
//         }
//       }
//     }
//   }

//   /* When client is disconnected, restart the ESP32 */
//   void onDisconnect(NimBLEClient *client) override {
//     Serial.println("Disconnected from BLE device, restarting");
//     ESP.restart();
//   }
// };

// static auto clientCallback = ClientCallback();

// // Search for client
// class ScanCallback : public NimBLEAdvertisedDeviceCallbacks {
//   void onResult(NimBLEAdvertisedDevice *advertised) {
//     auto macAddr = advertised->getAddress();

//     printf("Found BLE device %s\n", macAddr.toString().c_str());

//     if (macAddr != buttonMacAddress) {
//       return;
//     }

//     printf("Will try to connect to %s\n", macAddr.toString().c_str());
//     client = NimBLEDevice::createClient(macAddr);
//     client->setClientCallbacks(&clientCallback);
//     advertised->getScan()->stop();
//   }
// };

// static void onScanComplete(NimBLEScanResults results) {
//   printf("Scan complete\n");
//   // client->connect();
//   // keyboard.begin();
// }

// static auto scanCallback = ScanCallback();

// extern "C" void init_arduino() {
//   printf("Starting BLE scan\n");

//   scan->setAdvertisedDeviceCallbacks(&scanCallback);
//   scan->setInterval(SCAN_INTERVAL);
//   scan->setWindow(SCAN_WINDOW);
//   scan->setActiveScan(true);
//   // scan->setMaxResults(0);
//   scan->start(0, onScanComplete, false);
//   printf("Started BLE scan\n");
// }

// extern "C" void ble_keyboard_write(uint8_t c[2]) {
//   if (keyboard.isConnected()) {
//     keyboard.write(c);
//   }
// }

// extern "C" void ble_keyboard_print(const uint8_t *format) {
//   if (keyboard.isConnected()) {
//     keyboard.print(reinterpret_cast<const char *>(format));
//   }
// }

// extern "C" bool ble_keyboard_is_connected() {
//   return keyboard.isConnected();
// }
