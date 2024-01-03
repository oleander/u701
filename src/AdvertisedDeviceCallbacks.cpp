#ifndef ADVERTISED_DEVICE_CALLBACKS_H
#define ADVERTISED_DEVICE_CALLBACKS_H

#include <NimBLEAdvertisedDevice.h>

/** Define a class to handle the callbacks when advertisements are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    advertisedDevice->getScan()->stop();
  };
};

#endif // ADVERTISED_DEVICE_CALLBACKS_H
