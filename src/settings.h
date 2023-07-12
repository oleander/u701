// Outputs BLE messages to the serial monitor
// Has to be conbined with INFO
// #define DEBUG
#pragma once

// BLE scan settings
#define SCAN_INTERVAL 5000 // in ms
#define SCAN_WINDOW   100  // in ms

// Name of the broadcasted device used by the iPhone app
#define DEVICE_NAME "u701"

// The MAC address of the device to connect to
#define DEVICE_MAC "f7:97:ac:1f:f8:c0"

// Only used for debug
#define SERIAL_BAUD_RATE 115200

// Period of time in which to ignore additional level changes
#define DEBOUNCE_TICKS 50 // in ms

// Timeout used to distinguish single clicks from double clicks
#define CLICK_TICKS 500 // in ms

// Duration to hold a button to trigger a long press
#define LONG_PRESS_TICKS 800 // in ms

#define MAC_ADRESS    "f7:97:ac:1f:f8:c0"
#define WIFI_PASSWORD "11111111"
#define WIFI_SSID     "u701"
