#include <stdint.h>

extern "C" void ble_keyboard_write(uint8_t c[2]);
extern "C" void ble_keyboard_print(const uint8_t *format);
extern "C" bool ble_keyboard_is_connected();
extern "C" void handle_external_click_event(const uint8_t *event, size_t length);
extern "C" void setup_rust();
