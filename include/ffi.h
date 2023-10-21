#include <stdint.h>

extern "C" void sendCharacterViaBLEKeyboard(uint8_t c[2]);
extern "C" void printStringViaBLEKeyboard(const uint8_t *format);
extern "C" bool isBLEKeyboardConnected();
extern "C" void handleEventFromCpp(const uint8_t *event, size_t length);
extern "C" void setupRust();
extern "C" void handleBLEevents();
