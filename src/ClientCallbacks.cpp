#include "ClientCallbacks.h"
#include "utility.h"
#include <Arduino.h>
#include <ArduinoLog.h>
#include <NimBLEClient.h>
#include <NimBLEDevice.h>

extern SemaphoreHandle_t incommingClientSemaphore;

void ClientCallbacks::onConnect(NimBLEClient *pClient) {
  Log.traceln("Connected to Terrain Command");
  pClient->updateConnParams(120, 120, 0, 60);
}

void ClientCallbacks::onDisconnect(NimBLEClient *pClient) {
  restart("Disconnected from Terrain Command");
}

bool ClientCallbacks::onConnParamsUpdateRequest(NimBLEClient *_pClient, const ble_gap_upd_params *params) {
  if (params->itvl_min < 24) {
    return false;
  } else if (params->itvl_max > 40) {
    return false;
  } else if (params->latency > 2) {
    return false;
  } else if (params->supervision_timeout > 100) {
    return false;
  } else {
    return true;
  }
}

uint32_t onPassKeyRequest() {
  Log.traceln("Passkey request");
  return 111111;
};

bool onConfirmPIN(uint32_t pass_key) {
  Log.traceln("Confirm PIN %d", pass_key);
  return pass_key == 111111;
};

void ClientCallbacks::onAuthenticationComplete(ble_gap_conn_desc *desc) {
  if (desc->sec_state.encrypted) {
    xSemaphoreGive(incommingClientSemaphore);
    return;
  }

  Log.fatalln("Encrypt connection failed: %s", desc);
  Log.warningln("Will try to disconnect and reconnect");
  auto client = NimBLEDevice::getClientByID(desc->conn_handle);
  if (client != nullptr) {
    client->disconnect();
  }
  restart("Encrypt connection failed: %s", desc);
}
