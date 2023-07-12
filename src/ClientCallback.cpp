#include "ClientCallback.h"

void ClientCallback::onConnect(BLEServer *server) {
  Log.noticeln("Connected to device!");
}

void ClientCallback::onDisconnect(NimBLEClient *client) {
  restart("Disconnected from device");
}
