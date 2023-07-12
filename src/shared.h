#include <NimBLEDevice.h>

#pragma once

#define OTA_STATUS_ADDRESS 0

static NimBLEClient *client;
static NimBLEAdvertisedDevice *device;
static NimBLEUUID reportUUID("2A4D");
static NimBLEUUID cccdUUID("2902");
static NimBLEUUID hidService("1812");
