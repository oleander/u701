#include <stdint.h>
#include "keyboard_ffi.h"

extern "C" void transition_from_cpp(const uint8_t *event, size_t length);
extern "C" void setup_rust();
