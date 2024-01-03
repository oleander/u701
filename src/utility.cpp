#include <Arduino.h>
#include <vector>

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
  Serial.println(buffer.data());
  Serial.println("Will restart the ESP in 5 seconds");
  delay(5000);
  ESP.restart();
}
