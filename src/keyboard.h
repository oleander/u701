#include <ArduinoLog.h>

#pragma once

#include "settings.h"
#include <BleKeyboard.h>

void sendFnKeyPress(char letter);
void doubleClickHandler(void *p);
void multiClickHandler(void *p);
void setupButtons();
