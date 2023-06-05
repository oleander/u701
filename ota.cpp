#include "ota.h"
#include "print.h"
#include <ArduinoOTA.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h> // For ESP32

bool otaMode = false;

// Toggles the OTA mode
void toggleOTA() {
  otaMode = !otaMode;

  PRINTLN("OTA mode is " + String(otaMode ? "enabled" : "disabled"));
}

// Checks if OTA mode is enabled
bool isOTAEnabled() { return otaMode; }

// Sets up the OTA update functionality
void setupOTA() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("u701", "11111111");

  IPAddress myIP = WiFi.softAPIP();
  PRINT("AP IP address: ");
  PRINTLN(myIP);

  // ArduinoOTA.setHostname("u701");

  ArduinoOTA
      .onStart([]() {
        String type;

        if (ArduinoOTA.getCommand() == U_FLASH) {
          type = "sketch";
        } else {
          type = "filesystem";
          // Ensure there is no ongoing use of LittleFS before unmounting
          LittleFS.end();
        }

        PRINTLN("Start updating " + type);
      })
      .onEnd([]() {
        PRINTLN("\nEnd");
        delay(2000);
        ESP.restart();
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        PRINTF("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        PRINTF("Error[%u]: ", error);

        switch (error) {
        case OTA_AUTH_ERROR:
          PRINTLN("Auth Failed");
          break;
        case OTA_BEGIN_ERROR:
          PRINTLN("Begin Failed");
          break;
        case OTA_CONNECT_ERROR:
          PRINTLN("Connect Failed");
          break;
        case OTA_RECEIVE_ERROR:
          PRINTLN("Receive Failed");
          break;
        case OTA_END_ERROR:
          PRINTLN("End Failed");
          break;
        default:
          PRINTLN("Unknown Error");
        }
      });

  ArduinoOTA.begin();
}

// Handles OTA updates
void handleOTA() {
  if (otaMode) {
    ArduinoOTA.handle();
  }
}
