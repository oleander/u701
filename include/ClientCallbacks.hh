#ifndef CLIENTCALLBACKS_HH
#define CLIENTCALLBACKS_HH

#include <NimBLEClient.h>

namespace llvm_libc {
  class ClientCallbacks : public NimBLEClientCallbacks {
  public:
    void onConnect(NimBLEClient *pClient);
    void onDisconnect(NimBLEClient *pClient);
    bool onConnParamsUpdateRequest(NimBLEClient *_pClient, const ble_gap_upd_params *params);
    uint32_t onPassKeyRequest();
    bool onConfirmPIN(uint32_t pass_key);
  };
} // namespace llvm_libc
#endif // CLIENTCALLBACKS_HH
