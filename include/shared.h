#pragma once

#define OTA_STATUS_ADDRESS 0
#define PIN                33

static NimBLEUUID reportUUID("2A4D");
static NimBLEUUID cccdUUID("2902");
static NimBLEUUID hidService("1812");
static NimBLEUUID batteryServiceUUID("180F");
static NimBLEUUID batteryLevelCharUUID("2A19");

static NimBLEClient *client;

typedef u_int16_t ID;
