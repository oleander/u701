#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#include "settings.h"
#include "shared.h"
#include "state.h"
#include <BleKeyboard.h>
#include <OneButton.h>
#include <unordered_map>

// This pin is never used, but it is required by the OneButton library
#define KEY_INVALID 0xAA
#define KEY_SIRI 0xCF
#define KEY_ZOOM_OUT KEY_NUM_MINUS
#define KEY_ZOOM_IN KEY_NUM_PLUS
#define POINTER(p) (static_cast<uint16_t>(reinterpret_cast<uintptr_t>(p)))

extern BleKeyboard keyboard;
extern std::unordered_map<ID, OneButton *> buttons;
extern bool useKeyboardForLogging;
extern State state;

void sendNOPKey(ID id);
void sendFnKeyPress(char letter);
void doubleClickHandler(void *p);
void multiClickHandler(void *p);
void setupButtons();

BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);
