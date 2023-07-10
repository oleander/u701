#include "NimBLEAdvertisedDevice.h"
#include "NimBLEScan.h"
#include "settings.h"
#include "shared.h"
#include <ArduinoLog.h>
#include <string.h>

class ScanCallback : public NimBLEAdvertisedDeviceCallbacks {
public:
  void onResult(NimBLEAdvertisedDevice *advertised);
};
