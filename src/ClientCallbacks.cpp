// #include "ClientCallbacks.h"
// #include "utility.h"
// #include <Arduino.h>
// #include <NimBLEClient.h>

// extern SemaphoreHandle_t incommingClientSemaphore;

// void ClientCallbacks::onConnect(NimBLEClient *pClient) {
//   pClient->updateConnParams(120, 120, 0, 60);
// }

// void ClientCallbacks::onDisconnect(NimBLEClient *pClient) {
//   restart("Disconnected from Terrain Command");
// }

// bool ClientCallbacks::onConnParamsUpdateRequest(NimBLEClient *_pClient, const ble_gap_upd_params *params) {
//   return true;

//   if (params->itvl_min < 24) {
//     return false;
//   } else if (params->itvl_max > 40) {
//     return false;
//   } else if (params->latency > 2) {
//     return false;
//   } else if (params->supervision_timeout > 900) {
//     return false;
//   } else {
//     return true;
//   }
// }

// /** Pairing process complete, we can check the results in ble_gap_conn_desc */
// void ClientCallbacks::onAuthenticationComplete(ble_gap_conn_desc *desc) {
//   if (desc->sec_state.encrypted) {
//     xSemaphoreGive(incommingClientSemaphore);
//   } else {
//     restart("Encrypt connection failed: %s", desc);
//   }
// }
