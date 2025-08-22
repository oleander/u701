#include "ClientCallbacks.hh"
#include "utility.h"
#include "ffi.hh"

#include <Arduino.h>
#include <ArduinoLog.h>
#include <NimBLEClient.h>
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace llvm_libc {
  constexpr uint32_t PASS_KEY           = 111111;
  constexpr int MIN_INTERVAL            = 24;
  constexpr int MAX_INTERVAL            = 40;
  constexpr int MAX_LATENCY             = 2;
  constexpr int SUPERVISION_TIMEOUT     = 100;
  constexpr int CONNECTION_INTERVAL_MIN = 120;
  constexpr int CONNECTION_INTERVAL_MAX = 120;
  constexpr int CONNECTION_TIMEOUT      = 60;
  constexpr int VOLUME_UP_DELAY_MS    = 3000; // 3 seconds

  // FreeRTOS task to send volume up after delay
  void volumeUpTestTask(void * /* parameter */) {
    Log.noticeln("BLE Test: Will send volume up in 3 seconds...");
    vTaskDelay(pdMS_TO_TICKS(VOLUME_UP_DELAY_MS));
    Log.noticeln("BLE Test: Sending volume up command");
    send_volume_up_test();
    Log.noticeln("BLE Test: Volume up command sent");

    // Delete the task after completion
    vTaskDelete(nullptr);
  }

  void ClientCallbacks::onConnect(NimBLEClient *pClient) {
    Log.traceln("Connected to Terrain Command");

    // Start test task to send volume up after 3 seconds
    BaseType_t result = xTaskCreate(
      volumeUpTestTask,     // Task function
      "VolumeUpTest",       // Task name
      2048,                 // Stack size in words
      nullptr,              // Task parameter
      1,                    // Priority
      nullptr               // Task handle
    );

    if (result == pdPASS) {
      Log.noticeln("BLE Test: Started volume up test task");
    } else {
      Log.errorln("BLE Test: Failed to create volume up test task");
    }

    // pClient->updateConnParams(CONNECTION_INTERVAL_MIN, CONNECTION_INTERVAL_MAX, 0, CONNECTION_TIMEOUT);
  }

  void ClientCallbacks::onDisconnect(NimBLEClient * /* pClient */) {
    utility::reboot("Disconnected from Terrain Command");
  }

  bool ClientCallbacks::onConnParamsUpdateRequest(NimBLEClient * /* _pClient */, const ble_gap_upd_params *params) {
    // return (params->itvl_min < MIN_INTERVAL || params->itvl_max > MAX_INTERVAL || params->latency > MAX_LATENCY ||
    // params->supervision_timeout > SUPERVISION_TIMEOUT);
    return true;
  }

  uint32_t ClientCallbacks::onPassKeyRequest() {
    Log.traceln("Passkey request");
    return PASS_KEY;
  }

  bool ClientCallbacks::onConfirmPIN(uint32_t pass_key) {
    Log.traceln("Confirm PIN %d", pass_key);
    return pass_key == PASS_KEY;
  }
} // namespace llvm_libc
