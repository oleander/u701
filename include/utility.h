#ifndef UTILITY_H
#define UTILITY_H

#include <Arduino.h>
#include <string>

// how to make it const
namespace utility {
  constexpr auto RESTART_INTERVAL = 300;
  std::string stringFormat(const std::string &fmt, ...);
  void enableLED();
  void ledon();
  void ledoff();
  extern "C" void blinkled();
  template <typename... Args> void reboot(const std::string &msgFormat, Args &&...args);
} // namespace utility

#include "utility.tpp"
#endif // UTILITY_H
