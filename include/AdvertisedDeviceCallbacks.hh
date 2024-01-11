#ifndef ADVERTISEDDEVICECALLBACKS_HH
#define ADVERTISEDDEVICECALLBACKS_HH

#include <NimBLEAdvertisedDevice.h>

namespace llvm_libc {
  class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  public:
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) final;
  };
} // namespace __llvm_libc
#endif // ADVERTISEDDEVICECALLBACKS_HH
