#include "shared.h"
#include "settings.h"
#include "state.h"

OneButton *button = new OneButton(PIN, false);
State state = {0, OTA::IDLE, 0, button, false, Action::TICK};
