#include "AdvertisedDeviceCallbacks.hh"
#include <NimBLEAdvertisedDevice.h>

namespace llvm_libc {
  void AdvertisedDeviceCallbacks::onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    advertisedDevice->getScan()->stop();
  }
} // namespace __llvm_libc
