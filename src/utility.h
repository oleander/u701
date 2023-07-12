#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#include "shared.h"

void restart(const char *reason);
int32_t dataToInt(uint8_t *pData, size_t length);
