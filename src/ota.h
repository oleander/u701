#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#include "settings.h"
#include "utility.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiUdp.h>

void setupOTA();
void handleOTA();
