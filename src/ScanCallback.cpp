#include "ScanCallback.h"

void ScanCallback::onResult(NimBLEAdvertisedDevice *advertised) {
#ifndef RELEASE
  Log.notice(".");
#endif

  if (!advertised->isAdvertisingService(hidService)) return;

  auto addr = advertised->getAddress().toString();
  if (strcmp(addr.c_str(), DEVICE_MAC) != 0) return;

  device = advertised;

  Log.noticeln("Stopping scan ...");
  advertised->getScan()->stop();
}
