#include "debug.h"

// const char *getErrorMessage(uint8_t error_code) {
//   auto it = hci_error_codes.find(error_code);
//   if (it != hci_error_codes.end()) {
//     return it->second;
//   }
//   return nullptr;
// }

// void gattcEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
//                        esp_ble_gattc_cb_param_t *param) {
//   const char *error_msg = getErrorMessage(event);
//   if (error_msg) {
//     Log.noticeln("EVT: %s\n", error_msg);
//   } else {
//     Log.noticeln("EVT %x\n", event);
//   }
// }

// void gattsEventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
//                        esp_ble_gatts_cb_param_t *param) {
//   const char *error_msg = getErrorMessage(event);
//   if (error_msg) {
//     Log.noticeln("GATTS EVT: %s\n", error_msg);
//   } else {
//     Log.noticeln("GATTS EVT %x\n", event);
//   }
// }

// void enableDebug() {
// NimBLEDevice::setCustomGattcHandler(gattcEventHandler);
// NimBLEDevice::setCustomGattsHandler(gattsEventHandler);
// NimBLEDevice::setCustomGapHandler(my_gap_event_handler);
// }
