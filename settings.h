// Outputs BLE messages to the serial monitor
// Has to be conbined with INFO
// #define DEBUG
#pragma once

// Enables output to the serial monitor
// #define INFO
// #define IOS

// BLE scan settings
#define SCAN_INTERVAL 1250 // in ms
#define SCAN_WINDOW   650  // in ms
#define SCAN_DURATION 10   // in seconds

// Name of the broadcasted device used by the iPhone app
#define DEVICE_NAME "u701"

// The MAC address of the device to connect to
#define DEVICE_MAC "f7:97:ac:1f:f8:c0"

// How long do the red button need to be pressed to restart the ESP
#define RESTART_ACTIVATION_TIME 10000

// Only used for debug
#define SERIAL_BAUD_RATE 115200

// How long before the watchdog resets the ESP
#define WDT_TIMEOUT 300 // in seconds

// How often should the watchdog be reset
#define WDT_RESET_INTERVAL 60 // in seconds

// Period of time in which to ignore additional level changes
#define DEBOUNCE_TICKS 0 // in ms

// Timeout used to distinguish single clicks from double clicks
#define CLICK_TICKS 600 // in ms

// Duration to hold a button to trigger a long press
#define LONG_PRESS_TICKS 800 // in ms

#define WIFI_SSID     "u701"
#define WIFI_PASSWORD "11111111"

enum State {
  SCAN_DEVICE,
  CONNECT_TO_DEVICE,
  DEVICE_CONNECTED,
  INITIALIZE,
  FINISHED,
  DISCONNECTED,
  SETUP_OTA,
  HANDLE_OTA
};
static State state = SCAN_DEVICE;
