/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#include <ArduinoLog.h>

extern "C" void restart(const char *reason);
