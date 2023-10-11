#include "keyboard_ffi.h"
#include "keyboard.h"

size_t ble_keyboard_write(uint8_t c)
{
  return keyboard.write(c);
}

bool ble_keyboard_is_connected()
{
  return keyboard.isConnected();
}
