// #pragma once

// #include "utility.h"
// #include "ffi.h"
// #include <Arduino.h>
// #include <ArduinoLog.h>
// #include <esp_task_wdt.h>
// #include <vector>

// extern SemaphoreHandle_t outgoingClientSemaphore;

// #define RESTART_INTERVAL 5 // in seconds

// void restart(const char *format, ...) {
//   std::vector<char> buffer(256);

//   va_list args;
//   va_start(args, format);
//   int needed = vsnprintf(buffer.data(), buffer.size(), format, args);
//   va_end(args);

//   // Resize buffer if needed and reformat message
//   if (needed >= buffer.size()) {
//     buffer.resize(needed + 1);
//     va_start(args, format);
//     vsnprintf(buffer.data(), buffer.size(), format, args);
//     va_end(args);
//   }

//   Log.fatalln(buffer.data());
//   Log.fatalln("Will restart the ESP in %d seconds", RESTART_INTERVAL);
//   delay(RESTART_INTERVAL * 1000);
//   ESP.restart();
// }

// void removeWatchdog() {
//   Log.traceln("Remove watchdog");
//   esp_task_wdt_delete(NULL);
//   esp_task_wdt_deinit();
// }

// void updateWatchdogTimeout(uint32_t newTimeoutInSeconds) {
//   Log.traceln("Update watchdog timeout to %d seconds", newTimeoutInSeconds);
//   esp_task_wdt_deinit();
//   esp_task_wdt_init(newTimeoutInSeconds, true);
//   esp_task_wdt_add(NULL);
// }

// void onClientDisconnect(NimBLEServer *_server) {
//   restart("Client disconnected from the keyboard");
// }

// void disconnect(NimBLEClient *pClient, const char *format, ...) {
//   Log.traceln("Disconnect from Terrain Command");
//   pClient->disconnect();
//   restart(format);
// }

// void onClientConnect(ble_gap_conn_desc *_desc) {
//   Log.traceln("Connected to keyboard");
//   Log.traceln("Release keyboard semaphore (output) (semaphore)");
//   xSemaphoreGive(outgoingClientSemaphore);
// }

// void onEvent(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *data, size_t length, bool isNotify) {
//   if (!isNotify) {
//     return;
//   }

//   c_on_event(data, length);
// }
