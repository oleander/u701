#ifndef ADVERTISED_DEVICE_CALLBACKS_H
#define ADVERTISED_DEVICE_CALLBACKS_H

#include <NimBLEAdvertisedDevice.h>

class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
public:
  void onResult(NimBLEAdvertisedDevice *advertisedDevice);
};

#endif // ADVERTISED_DEVICE_CALLBACKS_H
