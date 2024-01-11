#ifndef app_UTILITY_TPP
#define app_UTILITY_TPP

#include <ArduinoLog.h> // Include ArduinoLog.h here
#include <BleKeyboard.h>
#include <cstdarg>
#include <cstdio>

namespace utility {
  constexpr char DEVICE_NAME[]         = "u701";
  constexpr char DEVICE_MANUFACTURER[] = "HVA";
  constexpr int DEVICE_BATTERY         = 42;

  SemaphoreHandle_t incommingClientSemaphore = xSemaphoreCreateBinary();
  SemaphoreHandle_t outgoingClientSemaphore  = xSemaphoreCreateBinary();
  BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);

  std::string stringFormat(const std::string &fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt.c_str(), args);
    va_end(args);
    return std::string(buffer);
  }

  template <typename... Args> void reboot(const std::string &msgFormat, Args &&...args) {
    std::string formattedMessage = stringFormat(msgFormat, std::forward<Args>(args)...);
    Log.fatalln(formattedMessage.c_str());
    Log.fatalln("Will restart the ESP in %d ms", RESTART_INTERVAL);
    delay(RESTART_INTERVAL);
    ESP.restart();
  }
} // namespace utility

#endif // app_UTILITY_TPP
