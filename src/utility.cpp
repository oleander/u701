#include "utility.h"

void restart(const char *reason) {
  Log.noticeln(reason);
  Log.noticeln("Restarting ESP32 ...");
  ESP.restart();
}
