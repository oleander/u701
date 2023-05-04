#include "settings.h"
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "keyboard.h"

void handleWatchdog();
void setupWatchdog();
void IRAM_ATTR wdt_isr_callback(void *arg);
