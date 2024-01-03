#include "utility.h"
#include <Arduino.h>
#include <ArduinoLog.h>
#include <esp_task_wdt.h>
#include <vector>

#define RESTART_INTERVAL 5 // in seconds

void restart(const char *format, ...) {
  std::vector<char> buffer(256);

  va_list args;
  va_start(args, format);
  int needed = vsnprintf(buffer.data(), buffer.size(), format, args);
  va_end(args);

  // Resize buffer if needed and reformat message
  if (needed >= buffer.size()) {
    buffer.resize(needed + 1);
    va_start(args, format);
    vsnprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);
  }

  Log.fatalln(buffer.data());
  // Print message and restart
  Log.fatalln("Will restart the ESP in %d seconds", RESTART_INTERVAL);
  delay(RESTART_INTERVAL * 1000);
  ESP.restart();
}

void removeWatchdog() {
  Log.traceln("Remove watchdog");
  esp_task_wdt_delete(NULL);
}

void updateWatchdogTimeout(uint32_t newTimeoutInSeconds) {
  Log.traceln("Update watchdog timeout to %d seconds", newTimeoutInSeconds);
  esp_task_wdt_deinit();
  esp_task_wdt_init(newTimeoutInSeconds, true);
  esp_task_wdt_add(NULL);
}

void onClientDisconnect(NimBLEServer *_server) {
  restart("Client disconnected from the keyboard");
}
