#include <stdint.h>
#include "keyboard_ffi.h"

extern "C" void transition_from_cpp(const uint8_t *event);
extern "C" void setup_rust();
