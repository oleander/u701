#include "ota.h"

void setupOTA() {
  Log.noticeln("Booting");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    restart("Connection Failed! Rebooting...", false);
  }

  ArduinoOTA.setPassword(WIFI_PASSWORD);

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
          type = "sketch";
        } else { // U_SPIFFS
          SPIFFS.end();
          type = "filesystem";
        }

        Log.noticeln("Start updating %s", type);
      })
      .onEnd([]() { restart("\nEnd", false); })
      .onProgress([](unsigned int progress, unsigned int total) { Log.noticeln("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error) {
        Log.noticeln("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Log.noticeln("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Log.noticeln("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Log.noticeln("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Log.noticeln("Receive Failed");
        else if (error == OTA_END_ERROR)
          Log.noticeln("End Failed");
      });

  Log.noticeln("Ready");
  Log.noticeln("IP address: %s", WiFi.localIP());
}

void handleOTA() {
  ArduinoOTA.handle();
}
