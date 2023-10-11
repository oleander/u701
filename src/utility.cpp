#include "utility.h"

void restart(const char *reason) {
  Log.noticeln(reason);

  if (client && client->isConnected()) {
    Log.noticeln("Disconnecting from device ...");
    if (client->disconnect()) {
      Log.noticeln("Could not disconnect from device!");
    } else {
      Log.noticeln("Failed to disconnect from device!");
    }
  }

  Log.noticeln("Restarting ESP32 ...");

  ESP.restart();
}

int32_t dataToInt(uint8_t *pData, size_t length) {
  int32_t result = 0;

  for (size_t i = 0; i < length; ++i) {
    result = (result << 8) | pData[i];
  }

  return result;
}
