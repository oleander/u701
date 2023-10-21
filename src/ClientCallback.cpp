#include "ClientCallback.h"

void ClientCallback::onConnect(NimBLEServer *server) {
  Log.noticeln("Connected to device!");
}

void ClientCallback::onDisconnect(NimBLEClient *client) {
  restart("Disconnected from device");
}
