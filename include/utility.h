#include <NimBLEServer.h>

void restart(const char *format, ...);
void removeWatchdog();
void updateWatchdogTimeout(uint32_t newTimeoutInSeconds);
void onClientDisconnect(NimBLEServer *_server);
