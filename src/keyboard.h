#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#pragma once

#include "settings.h"
#include <BleKeyboard.h>

// This pin is never used, but it is required by the OneButton library
extern BleKeyboard keyboard;
extern bool useKeyboardForLogging;

// void sendNOPKey(ID id);
void sendFnKeyPress(char letter);
void doubleClickHandler(void *p);
void multiClickHandler(void *p);
void setupButtons();
