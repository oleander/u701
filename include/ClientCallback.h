/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR

#include "NimBLEClient.h"
#include "utility.h"
#include <ArduinoLog.h>
#include <NimBLEServer.h>

class ClientCallback : public NimBLEClientCallbacks {
public:
  void onConnect(NimBLEServer *pServer);
  void onDisconnect(NimBLEClient *client);
};
