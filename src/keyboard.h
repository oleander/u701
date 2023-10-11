#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#pragma once

#include "settings.h"
#include <BleKeyboard.h>

void sendFnKeyPress(char letter);
void doubleClickHandler(void *p);
void multiClickHandler(void *p);
void setupButtons();
