#include "ota.h"
#include <NimBLEDevice.h>

#pragma once

#define OTA_STATUS_ADDRESS 0
#define PIN                33

static NimBLEUUID reportUUID("2A4D");
static NimBLEUUID cccdUUID("2902");
static NimBLEUUID hidService("1812");

static NimBLEAdvertisedDevice *device;
static NimBLECharacteristic *char1;
static NimBLECharacteristic *char2;
static NimBLEAdvertising *advert;
static NimBLEService *service1;
static NimBLEService *service2;
static NimBLEClient *client;
static NimBLEServer *server;

typedef u_int16_t ID;
