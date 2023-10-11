#include <stdint.h>

extern "C" void ble_keyboard_write(uint8_t c[2]);
extern "C" void ble_keyboard_print(uint8_t c);
extern "C" bool ble_keyboard_is_connected();
