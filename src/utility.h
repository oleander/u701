#include "shared.h"
#include <ArduinoLog.h>

void restart(const char *reason, bool _otaStatus);
int32_t dataToInt(uint8_t *pData, size_t length);
