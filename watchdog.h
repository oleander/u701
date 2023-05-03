#include <Arduino.h>
#include <esp_task_wdt.h>

#define WDT_RESET_INTERVAL 2000
#define WDT_TIMEOUT        120

void handleWatchdog();
void setupWatchdog();
void IRAM_ATTR wdt_isr_callback(void *arg);
