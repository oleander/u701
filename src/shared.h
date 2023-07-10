#include <NimBLEDevice.h>

#pragma once

static NimBLEClient *client;
static NimBLEAdvertisedDevice *device;
static NimBLEUUID reportUUID("2A4D");
static NimBLEUUID cccdUUID("2902");
static NimBLEUUID hidService("1812");

RTC_NOINIT_ATTR static int otaStatus;
