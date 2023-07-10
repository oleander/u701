#include "settings.h"
#include <ArduinoLog.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiUdp.h>

void setupOTA();
void handleOTA();
