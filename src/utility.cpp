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
