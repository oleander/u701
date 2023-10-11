#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#pragma once

#include "settings.h"
#include "state.h"
#include <BleKeyboard.h>

// This pin is never used, but it is required by the OneButton library
#define KEY_INVALID  0xAA
#define KEY_SIRI     0xCF
#define KEY_ZOOM_OUT KEY_NUM_MINUS
#define KEY_ZOOM_IN  KEY_NUM_PLUS
#define POINTER(p)   (static_cast<uint16_t>(reinterpret_cast<uintptr_t>(p)))

extern BleKeyboard keyboard;
extern bool useKeyboardForLogging;

// void sendNOPKey(ID id);
void sendFnKeyPress(char letter);
void doubleClickHandler(void *p);
void multiClickHandler(void *p);
void setupButtons();
