#include "utility.h"
#include <BleKeyboard.h>

namespace llvm_libc {
  extern "C" void ble_keyboard_write(uint8_t character[2]) {
    utility::keyboard.write(character);
  }

  extern "C" void ble_keyboard_print(const uint8_t *format) {
    const char *formattedString = reinterpret_cast<const char *>(format);
    utility::keyboard.print(formattedString);
  }

  extern "C" void send_volume_down_test() {
    // Volume down command: [64, 0] as defined in machine/src/constants.rs
    uint8_t volumeDown[2] = {64, 0};
    utility::keyboard.write(volumeDown);
  }
} // namespace llvm_libc
