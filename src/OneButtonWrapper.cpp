#include "OneButton.h"

extern "C" {
OneButton *one_button_new(int pin, bool activeLow, bool pullupActive) {
  return new OneButton(pin, activeLow, pullupActive);
}

// Wrap other necessary functions from the OneButton class here
}