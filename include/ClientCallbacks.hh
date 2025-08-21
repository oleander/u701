#ifndef CLIENTCALLBACKS_HH
#define CLIENTCALLBACKS_HH

#include <NimBLEClient.h>

namespace llvm_libc {
  class ClientCallbacks : public NimBLEClientCallbacks {
  public:
    void onConnect(NimBLEClient *pClient) override;
    void onDisconnect(NimBLEClient *pClient) override;
    bool onConnParamsUpdateRequest(NimBLEClient *_pClient, const ble_gap_upd_params *params) override;
    uint32_t onPassKeyRequest() override;
    bool onConfirmPIN(uint32_t pass_key) override;
  };
} // namespace llvm_libc
#endif // CLIENTCALLBACKS_HH
