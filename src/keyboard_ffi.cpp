#include "keyboard_ffi.h"
#include "keyboard.h"

// fixed size array (2) of uint8_t
void ble_keyboard_write(uint8_t c[2])
{
  keyboard.write(c);
}

void ble_keyboard_print(uint8_t c)
{
  keyboard.print(c);
}

bool ble_keyboard_is_connected()
{
  return keyboard.isConnected();
}
