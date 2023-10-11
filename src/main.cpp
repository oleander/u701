#include <ArduinoLog.h>

/* Removes warnings */
#undef LOG_LEVEL_INFO
#undef LOG_LEVEL_ERROR
#define WDT_TIMEOUT 10 * 60

#include "ClientCallback.h"
#include "keyboard.h"
#include "rust_interface.h"
#include "ota.h"
#include "settings.h"
#include "shared.h"
#include "utility.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUtils.h>
#include <OneButton.h>

constexpr auto COMMAND_MAP_SERVICE_UUID = "19B10010-E8F2-537E-4F6C-D104768A1214";
constexpr auto COMMAND_MAP_CHAR_UUID = "19B10011-E8F2-537E-4F6C-D104768A1214";
constexpr auto COMMAND_SERVICE_UUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
constexpr auto COMMAND_CHAR_UUID = "19B10001-E8F2-537E-4F6C-D104768A1214";

const auto address = NimBLEAddress(DEVICE_MAC, 1);
const int SEVEN_MINUTES = 420000000;
static hw_timer_t *timer = NULL;

const auto RESTART_CMD = "restart";
const auto UPDATE_CMD = "update";
uint32_t connectedAt;

bool canProcessEvents()
{
  return (millis() - connectedAt) > 5000;
}

class MyCallbacks : public NimBLECharacteristicCallbacks
{
  void onWrite(NimBLECharacteristic *characteristic) override
  {
    std::string cmd = characteristic->getValue();
    if (cmd.length() == 0)
      return;
    Log.noticeln("Received value: %s\n", cmd.c_str());

    if (cmd == RESTART_CMD)
    {
      restart("Restart command received");
    }
    else if (cmd == UPDATE_CMD)
    {
      characteristic->setValue("Toggle OTA");
      state.action = Action::INIT_OTA;
      auto scan = NimBLEDevice::getScan();
      if (scan->isScanning())
        scan->stop();
    }
    else
    {
      characteristic->setValue("Unknown command");
    }
  }
};

/* Add function isActive to the State struct */
static void onEvent(BLERemoteCharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify)
{
  // if (!canProcessEvents())
  //   return;
  // if (characteristic->getUUID() != reportUUID)
  //   return;

  // an array of uint8_t
  Log.noticeln("Received data: %s\n", data);
  Log.noticeln("Received length: %d\n", length);
  Log.noticeln("Received isNotify: %d\n", isNotify);

  transition_from_cpp(data);
  // if (length != 4)
  //   return;
  // if (!isNotify)
  //   return;

  // auto newID = dataToInt(data, length);
  // auto oldState = state.isActive();
  // auto newState = !!newID;

  // Log.noticeln("Button %x set from %d -> %d", newID, oldState, newState);
  // /* Button is pressed */
  // if (!oldState && newState)
  // {
  //   Log.noticeln("Button was pressed");
  //   digitalWrite(LED_BUILTIN, HIGH);

  //   auto it = buttons.find(newID);
  //   if (it != buttons.end())
  //   {
  //     state.button = it->second;
  //     state.id = newID;
  //     state.setActive();
  //   }
  //   else
  //   {
  //     Log.noticeln("[BUG] Unknown button ID: %x", newID);
  //   }
  //   /* Button is released */
  // }
  // else if (oldState && !newState)
  // {
  //   Log.noticeln("Button was released");
  //   state.setInactive();
  //   digitalWrite(LED_BUILTIN, LOW);
  // }
  // else
  // {
  //   Log.noticeln("[BUG] Unknown old state: %d, new state %d, ID: %x", oldState, newState, newID);
  // }
}

void setupKeyboard()
{
  Log.noticeln("Enable Keyboard");
  keyboard.begin();
  keyboard.setDelay(13);
}

void setupSerial()
{
  Serial.begin(SERIAL_BAUD_RATE);
#ifdef RELEASE
  Log.begin(LOG_LEVEL_SILENT, &Serial);
#else
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
#endif

  Log.noticeln("Starting ESP32 ...");
}

/**
 * Sets up the client to connect to the BLE device with the specified MAC address.
 * If the connection fails or no services/characteristics are found, the device will restart.
 */
void setupClient()
{
  if (!device)
  {
    if (state.action == Action::TICK)
    {
      restart("Device not found, will reboot");
    }
    else
    {
      Log.noticeln("Device not found, still enter the loop");
      return;
    }
  }

  client = NimBLEDevice::createClient();
  client->setClientCallbacks(new ClientCallback());

  Log.noticeln("Connecting to %s ...", address.toString().c_str());
  if (!client->connect(device))
  {
    restart("Timeout connecting to the device");
  }

  Log.noticeln("Discovering services ...");
  auto services = client->getServices(true);
  if (services->empty())
  {
    restart("[BUG] No services found, will retry");
  }

  for (auto &service : *services)
  {
    if (!service->getUUID().equals(hidService))
      continue;

    Log.noticeln("Discovering characteristics ...");
    auto characteristics = service->getCharacteristics(true);
    if (characteristics->empty())
    {
      restart("[BUG] No characteristics found");
    }

    for (auto &characteristic : *characteristics)
    {
      if (!characteristic->getUUID().equals(reportUUID))
        continue;

      if (!characteristic->canNotify())
      {
        restart("[BUG] Characteristic cannot notify");
      }

      auto status = characteristic->subscribe(true, onEvent, true);
      if (!status)
      {
        restart("[BUG] Failed to subscribe to notifications");
      }

      Log.noticeln("Subscribed to notifications");
      return;
    }
  }

  restart("[BUG] No report characteristic found");
}

/**
 * Interrupt service routine that is triggered by a timer after seven minutes.
 * Calls the restart function with the message "OTA update failed" and false as the second argument.
 */
void IRAM_ATTR onTimer()
{
  restart("OTA update failed");
}

/**
 * Sets up a timer to trigger an interrupt after seven minutes.
 * The interrupt will call the onTimer function.
 * Used to reboot the ESP32 after seven minutes of OTA.
 */
void setupTimer()
{
  timer = timerBegin(0, 40, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, SEVEN_MINUTES, false);
}

class Callbacks : public NimBLEAdvertisedDeviceCallbacks
{
  void onResult(NimBLEAdvertisedDevice *advertised)
  {
    if (advertised->getAddress() == address)
    {
      Log.noticeln("Found device device");
      device = advertised;
      advertised->getScan()->stop();
    }
    else
    {
      Log.noticeln("Found device %s", advertised->getAddress().toString().c_str());
    }
  }
};

/**
 * Sets up the BLE scan to search for the device with the specified MAC address.
 * If the device is found, the scan will stop and the client will be set up.
 * The scan interval is set high to save power
 */
void setupScan()
{
  Log.noticeln("Starting BLE scan ...");

  auto scan = NimBLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new Callbacks());
  scan->setInterval(SCAN_INTERVAL);
  scan->setWindow(SCAN_WINDOW);
  scan->setActiveScan(true);
  scan->start(0, false);

  Log.noticeln("Scan finished");
}

void setupBLE()
{
  Log.noticeln("Starting BLE server...");

  NimBLEDevice::init(DEVICE_NAME);

  server = NimBLEDevice::createServer();
  service1 = server->createService(COMMAND_SERVICE_UUID);
  char1 = service1->createCharacteristic(COMMAND_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

  char1->setCallbacks(new MyCallbacks());
  service1->start();

  service2 = server->createService(COMMAND_MAP_SERVICE_UUID);
  char2 = service2->createCharacteristic(COMMAND_MAP_CHAR_UUID, NIMBLE_PROPERTY::READ);
  char2->setValue("Available commands:restart update");
  service2->start();

  advert = NimBLEDevice::getAdvertising();
  advert->addServiceUUID(COMMAND_SERVICE_UUID);
  advert->addServiceUUID(COMMAND_MAP_SERVICE_UUID);
  advert->start();
}

void setup()
{

  connectedAt = millis();

  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, HIGH);
  delay(3000);
  digitalWrite(LED_BUILTIN, LOW);

  setupSerial();
  setupButtons();
  setupBLE();
  setup_rust();
  setupKeyboard();
  setupScan();
  setupClient();
  setupTimer();

  state.action = Action::WAIT_FOR_PHONE;
  Log.noticeln("Setup completed\n");
}

void loop()
{
  switch (state.action)
  {
  case Action::RESTART:
    restart("Restart action triggered");
    break;
  case Action::INIT_OTA:
    Log.noticeln("Enabling OTA from BLE cmd...");
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
    Log.noticeln("IP address: %s", WiFi.softAPIP().toString().c_str());
    ArduinoOTA.begin();
    Log.noticeln("OTA enabled");
    timerAlarmEnable(timer);
    Log.noticeln("Timer enabled");
    state.action = Action::LOOP_OTA;
    break;
  case Action::LOOP_OTA:
    ArduinoOTA.handle();
    break;
  case Action::WAIT_FOR_PHONE:
    if (keyboard.isConnected())
    {
      Log.noticeln("Phone connected");
      connectedAt = millis();
      state.action = Action::TICK;
    }
  case Action::TICK:
    for (auto &[id, btn] : buttons)
    {
      if (id == state.id && state.isActive())
      {
        btn->tick(true);
      }
      else
      {
        btn->tick(false);
      }
    }
    break;
  default:
    Log.infoln("Wrong action: %s", state.action);
    break;
  }
}
