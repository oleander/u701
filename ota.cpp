#include "ota.h"
#include "print.h"
#include <ArduinoOTA.h>
#include <FS.h>
#include <LittleFS.h>

bool otaMode = false;

void toggleOTA() {
  otaMode = !otaMode;
  PRINTLN("OTA mode is " + String(otaMode ? "enabled" : "disabled"));
}

bool isOTAEnabled() { return otaMode; }

void setupOTA() {
  ArduinoOTA.setHostname("u701");

  ArduinoOTA.onStart([]() {
    String type;

    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
      LittleFS.end();
    }

    PRINTLN("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() { PRINTLN("\nEnd"); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    PRINTF("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    PRINTF("Error[%u]: ", error);

    if (error == OTA_AUTH_ERROR)
      PRINTLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      PRINTLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      PRINTLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      PRINTLN("Receive Failed");
    else if (error == OTA_END_ERROR)
      PRINTLN("End Failed");
  });

  ArduinoOTA.begin();
}

void handleOTA() {
  if (otaMode) {
    ArduinoOTA.handle();
  }
}
