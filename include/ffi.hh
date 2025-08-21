#ifndef FFI_HH
#define FFI_HH

#include <cstddef>
#include <cstdint>

namespace llvm_libc {
  extern "C" {
  void c_on_event(const uint8_t *event, size_t length);
  void ble_keyboard_print(const uint8_t *format);
  void ble_keyboard_write(uint8_t character[2]);
  void send_volume_down_test();
  }
} // namespace __llvm_libc

#endif // FFI_HH
