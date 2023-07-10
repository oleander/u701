#include "ClientCallback.h"

void ClientCallback::onConnect(BLEServer *pServer) {
  Log.noticeln("Connected to device!");
}

void ClientCallback::onDisconnect(NimBLEClient *client) {
  restart("Disconnected from device, restarting ESP32 ...", false);
}
