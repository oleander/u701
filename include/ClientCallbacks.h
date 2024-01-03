#include <NimBLEClient.h>

class ClientCallbacks : public NimBLEClientCallbacks {
public:
  bool onConnParamsUpdateRequest(NimBLEClient *_pClient, const ble_gap_upd_params *params);
  void onAuthenticationComplete(ble_gap_conn_desc *desc);
  void onDisconnect(NimBLEClient *pClient);
  void onConnect(NimBLEClient *pClient);
};
