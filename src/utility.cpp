#include <Arduino.h>
#include <ArduinoLog.h>
#include <vector>

#define RESTART_INTERVAL 5 // in seconds

void restart(const char *format, ...) {
  std::vector<char> buffer(256);

  va_list args;
  va_start(args, format);
  int needed = vsnprintf(buffer.data(), buffer.size(), format, args);
  va_end(args);

  // Resize buffer if needed and reformat message
  if (needed >= buffer.size()) {
    buffer.resize(needed + 1);
    va_start(args, format);
    vsnprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);
  }

  // Print message and restart
  Log.fatalln(buffer.data());
  Log.fatalln("Will restart the ESP in %d seconds", RESTART_INTERVAL);
  delay(RESTART_INTERVAL * 1000);
  ESP.restart();
}
