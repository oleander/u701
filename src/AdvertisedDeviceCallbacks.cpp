#include "AdvertisedDeviceCallbacks.h"
#include <NimBLEAdvertisedDevice.h>

void AdvertisedDeviceCallbacks::onResult(NimBLEAdvertisedDevice *advertisedDevice) {
  advertisedDevice->getScan()->stop();
}
