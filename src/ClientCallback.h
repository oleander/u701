#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#include "BLEServer.h"
#include "NimBLEClient.h"
#include "utility.h"

class ClientCallback : public NimBLEClientCallbacks {
public:
  void onConnect(BLEServer *pServer);
  void onDisconnect(NimBLEClient *client);
};
