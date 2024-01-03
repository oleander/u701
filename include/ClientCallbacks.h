#include <NimBLEClient.h>

class ClientCallbacks : public NimBLEClientCallbacks {
public:
  bool onConnParamsUpdateRequest(NimBLEClient *_pClient, const ble_gap_upd_params *params);
  void onAuthenticationComplete(ble_gap_conn_desc *desc);
  uint32_t onPassKeyRequest();
  bool onConfirmPIN(uint32_t pass_key);
  void onDisconnect(NimBLEClient *pClient);
  void onConnect(NimBLEClient *pClient);
};
