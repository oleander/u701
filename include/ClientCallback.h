#include "BLEServer.h"
#include "NimBLEClient.h"
#include "utility.h"
#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

class ClientCallback : public NimBLEClientCallbacks {
public:
  void onConnect(BLEServer *pServer);
  void onDisconnect(NimBLEClient *client);
};
