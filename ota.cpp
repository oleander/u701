#include "ota.h"
#include "print.h"
#include "settings.h"

void handleOTA() { ArduinoOTA.handle(); }

void setupOTA() {
  PRINTLN("Setup OTA");
  
  // Configure the ESP32 as an access point
  WiFi.mode(WIFI_AP);

  // Initialize the access point with the given credentials
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

  // Print the access point IP address
  IPAddress myIP = WiFi.softAPIP();
  PRINT("AP IP address: ");
  PRINTLN(myIP);

  ArduinoOTA.setPassword(WIFI_PASSWORD);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    PRINTLN("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    PRINTLN("\nEnd");
    PRINTLN("\nRestarting ESP ...");
    delay(2000);
    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    PRINTF("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    PRINTF("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      PRINTLN("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      PRINTLN("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      PRINTLN("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      PRINTLN("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      PRINTLN("End Failed");
    }
  });

  ArduinoOTA.begin();
}
