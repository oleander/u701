
// #include "ffi.h"
// #include "settings.h"

// // #include <ArduinoLog.h>
// // #include <BleKeyboard.h>
// #include <NimBLEDevice.h>
// #include <NimBLEScan.h>
// #include <NimBLEUtils.h>
// #include <esp_task_wdt.h>

// #define WDT_TIMEOUT 30
// // #include <ArduinoOTA.h>
// // #include <WiFi.h>

// IPAddress ip(192, 168, 4, 1);
// IPAddress gateway(192, 168, 4, 1);
// IPAddress subnet(255, 255, 255, 0);

// static NimBLEUUID reportUUID("2A4D");
// static NimBLEUUID cccdUUID("2902");
// static NimBLEUUID hidService("1812");
// static NimBLEUUID batteryServiceUUID("180F");
// static NimBLEUUID batteryLevelCharUUID("2A19");

// static NimBLEClient *client;

// /* Removes warnings */
// #undef LOG_LEVEL_INFO
// #undef LOG_LEVEL_ERROR

// const auto buttonMacAddress = NimBLEAddress(DEVICE_MAC, 1);

// // BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);

// void restart(const char *reason) {
//   // Log.noticeln(reason);
//   // Log.noticeln("Restarting ESP32 ...");
//   ESP.restart();
// }

// class ClientCallback : public NimBLEClientCallbacks {
//   void onConnect(NimBLEClient *client) override {
//     esp_task_wdt_reset();
//     // Log.noticeln("Connected to device!");
//   }

//   void onDisconnect(NimBLEClient *client) override {
//     restart("Disconnected from device");
//   }
// };

// extern "C" void ble_keyboard_write(uint8_t c[2]) {
//   // if (keyboard.isConnected()) {
//   // keyboard.write(c);
//   // }
// }

// extern "C" void ble_keyboard_print(const uint8_t *format) {
//   // if (keyboard.isConnected()) {
//   //   keyboard.print(reinterpret_cast<const char *>(format));
//   // }
// }

// extern "C" bool ble_keyboard_is_connected() {
//   // return keyboard.isConnected();
// }

// /* Add function isActive to the State struct */
// static void handleButtonClick(BLERemoteCharacteristic *_, uint8_t *data, size_t length, bool isNotify) {
//   // if (length != 4) {
//   //   .traceln("Received length should be 4, got %d (will continue anyway)", length);
//   // }

//   if (!isNotify) {
//     // Log.traceln("Received invalid isNotify: %d (expected true)", isNotify);
//     return;
//   }

//   // Log.traceln("[Click] Received length: %d", length);
//   // Log.traceln("[Click] Received isNotify: %d", isNotify);

//   c_on_event(data, length);
// }

// // void initializeKeyboard() {
// //   esp_task_wdt_reset();
// //   // Log.noticeln("Enable Keyboard");

// //   keyboard.setBatteryLevel(100);
// //   keyboard.setDelay(12);
// //   keyboard.begin();

// //   while (!keyboard.isConnected()) {
// //     esp_task_wdt_reset();
// //     delay(10);
// //   }
// // }

// void initializeSerialCommunication() {
//   esp_task_wdt_reset();
//   Serial.begin(SERIAL_BAUD_RATE);
//   // Log.begin(LOG_LEVEL_VERBOSE, &Serial);
//   // Log.setLevel(LOG_LEVEL_VERBOSE);
//   // Log.noticeln("Starting ESP32 ...");
//   esp_task_wdt_reset();
// }

// /**
//  * Sets up the client to connect to the BLE device with the specified MAC address.
//  * If the connection fails or no services/characteristics are found, the device will restart.
//  */
// void connectToClientDevice() {
//   esp_task_wdt_reset();
//   // Log.noticeln("[Connecting] to Terrain Command ...");

//   if (client == nullptr) {
//     restart("Device not found, will reboot");
//   }

//   static ClientCallback clientCallbackInstance;
//   client->setClientCallbacks(&clientCallbackInstance);
//   if (client->isConnected()) {
//     // return Log.noticeln("Already connected to device");
//     return;
//   }

//   if (!client->connect()) {
//     restart("Could not connect to the Terrain Command");
//   }

//   // Log.noticeln("Discovering services ...");
//   for (auto &service: *client->getServices(true)) {
//     // Log.noticeln("Discovering characteristics ...");
//     for (auto &characteristic: *service->getCharacteristics(true)) {
//       auto currentServiceUUID = service->getUUID().toString().c_str();
//       auto currentCharUUID    = characteristic->getUUID().toString().c_str();

//       esp_task_wdt_reset();

//       // Register for click events
//       if (!service->getUUID().equals(hidService)) {
//         // Log.warningln("[Click] Unknown report service: %X", currentServiceUUID);
//       } else if (!characteristic->getUUID().equals(reportUUID)) {
//         // Log.warningln("[Click] Unknown report characteristic: %X", currentCharUUID);
//       } else if (!characteristic->canNotify()) {
//         // Log.warningln("[Click] Cannot subscribe to notifications: %X", currentCharUUID);
//       } else if (!characteristic->subscribe(true, handleButtonClick, false)) {
//         // Log.errorln("[Click] [Bug] Failed to subscribe to notifications: %X", currentCharUUID);
//       } else {
//         // return Log.noticeln("[Click] Subscribed to notifications: %X", currentCharUUID);
//         return;
//       }
//     }
//   }

//   // Log.noticeln("Subcribed to all characteristics");
//   esp_task_wdt_reset();
// }

// class Callbacks : public NimBLEAdvertisedDeviceCallbacks {
//   void onResult(NimBLEAdvertisedDevice *advertised) {
//     auto macAddr = advertised->getAddress();

//     if (macAddr != buttonMacAddress) {
//       Serial.print(".");
//       return;
//     }

//     client = NimBLEDevice::createClient(macAddr);
//     advertised->getScan()->stop();

//     // Log.noticeln("[SCAN] Terrain Command found");
//   }
// };

// /**
//  * Sets up the BLE scan to search for the device with the specified MAC address.
//  * If the device is found, the scan will stop and the client will be set up.
//  * The scan interval is set high to save power
//  */
// void startBLEScanForDevice() {
//   // Log.noticeln("Starting BLE scan ...");

//   auto scan = NimBLEDevice::getScan();
//   static Callbacks scanCallbackInstance;
//   scan->setAdvertisedDeviceCallbacks(&scanCallbackInstance);
//   scan->setInterval(SCAN_INTERVAL);
//   scan->setWindow(SCAN_WINDOW);
//   scan->setActiveScan(true);
//   scan->start(0, false);
// }

// void setupWatchdog() {
//   // Log.noticeln("Setting up watchdog ...");
//   esp_task_wdt_init(WDT_TIMEOUT, true);
//   esp_task_wdt_add(nullptr);
// }

// void disableWatchdog() {
//   // Log.noticeln("Disabling watchdog ...");
//   esp_task_wdt_delete(nullptr);
// }

// void setupBle() {
//   esp_task_wdt_reset();
//   // NimBLEDevice::init(DEVICE_NAME);
// }

// extern "C" void init_arduino() {
//   setupWatchdog();
//   setupBle();
//   // initializeKeyboard();
//   initializeSerialCommunication();
//   disableWatchdog();
//   startBLEScanForDevice();
//   setupWatchdog();
//   connectToClientDevice();
//   disableWatchdog();
// }
