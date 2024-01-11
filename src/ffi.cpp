#include "utility.h"
#include <BleKeyboard.h>

namespace app {
  extern "C" void ble_keyboard_write(uint8_t character[2]) {
    utility::keyboard.write(character);
  }

  extern "C" void ble_keyboard_print(const uint8_t *format) {
    const char *formattedString = reinterpret_cast<const char *>(format);
    utility::keyboard.print(formattedString);
  }
} // namespace app
