#include "watchdog.h"
#include "print.h"

unsigned long lastResetTime = 0;
esp_timer_handle_t wdt_timer;

void setupWatchdog() {
  PRINTLN("Configuring WDT...");

  esp_timer_create_args_t wdt_timer_config;
  wdt_timer_config.callback        = wdt_isr_callback;
  wdt_timer_config.arg             = NULL;
  wdt_timer_config.dispatch_method = ESP_TIMER_TASK;
  wdt_timer_config.name            = "wdt_timer";

  esp_timer_create(&wdt_timer_config, &wdt_timer);
  esp_timer_start_periodic(wdt_timer, WDT_TIMEOUT * 1000000); // microseconds
}

void handleWatchdog() {
  unsigned long currentTime = millis();

  if (currentTime - lastResetTime < WDT_RESET_INTERVAL) return;

  esp_timer_stop(wdt_timer);
  esp_timer_start_periodic(wdt_timer, WDT_TIMEOUT * 1000000);

  lastResetTime = currentTime;
}

void IRAM_ATTR wdt_isr_callback(void *arg) { esp_task_wdt_reset(); }
