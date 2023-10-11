# u701-rs

A BLE proxy written in Rust making the terrain command device work with iOS.

## Setup

1. `rustup update`
2. `cargo update`

### ESP-IDF

```bash
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd ~/esp/esp-idf
./install.sh esp32c3
```

## Hardware

* buttons: 8 buttons sending keyboard events over BLE
* iphone: the phone being controlled by the buttons
* esp32: the microcontroller responsible remapping the buttons to keyboard events

## Problem

The buttons does not work with iOS, only android. The ESP32 is responsible for remapping the android keyboard events to iOS keyboard events. The mapping is dynamic and can be altered by the user over BLE.

Each button allows for 3 different click types: single, double, and long. Each click type can be mapped to a different keyboard event, either a media key or a letter key. A media key is a key that controls the media player on the iPhone, e.g. Play/Pause. A letter key is a key that sends a letter to the iPhone, e.g. 'A' and can be used to trigger iOS shortcuts.

## Services

* receiver: Receives BLE events from the buttons
* keyboard: Sends BLE events to the iPhone
* config: Stores and retrieves key/value pairs
* button: Translates BLE events to button clicks
* blenvs: Allows for remote configuration of the buttons over BLE

## Dataflow

1. BLE events are received from the terrain command device from now on called "the buttons"
2. The BLE events are translated click events
   1. Down -> Up => Click
3. Events are bound to a key representing an BLE keyboard event sent to the iPhone (e.g. "Play/Pause")

## Flow

1. receiver -> button -> config -> button -> keyboard
2. blenvs             -> config

1. Is triggered by a physical button click, e.g. down -> up
2. Is triggered by by the user over BLE used to configure the buttons

## Example

### BLE button click

> Button 3 is clicked and previously mapped to Play/Pause

1. [receiver] Button 0x5510 event down is received (0x5510, "down")
2. [receiver] Button 0x5510 event up is received (0x5510, "up")
3. [button] Determents that button 0x5510 has been clicked (0x5510, "single")
4. [button] Maps button 0x5510 to button index 3
5. [button] Maps click event "single" to 0
5. [button] Combines button index & click event into 3 & 0 into 30 (string)
6. [config] Retrieves value for key 30 from the key/value store and gets (0x02)
7. [button] Maps 0x00 to BLE event type 0 (HID media key)
7. [button] Maps 0x02 to payload 2 (Media key: Play/Pause)
8. [button] Tells the keyboard to send HID event Play/Pause (MediaKey Play/Pause)
9. [keyboard] Sends MediaKey Play/Pause

### Update configuration

> User remaps the second button to send the letter 'A' on single click

1. [blenvs] Creates a write BLE service that allows for an i32 input
   1. Button index: 0 - 7
   2. Click type: 0 - 2 (single, double, long)
   3. BLE event type: 0 - 1 (HID media key, letter key)
   4. Payload: 0 - 255 (HID media key, letter key)
2. [blenvs] Receives a write event (0x1112)
   1. Button 1: 1
   2. Single click: 1
   3. Letter key: 1
   4. Play/Pause: 2
3. [blenvs] Combines keys into 1 & 1 into 11 (string)
4. [blenvs] Combines event types & payload into 1 & 2 into 12 (u16)
5. [config] Stores value 12 for key 11 in the key/value store

### Read configuration

> User wants to know what the second button is mapped to

1. [blenvs] Creates a read BLE service that allows for i16 input & i16 output
  * Input:
     1. Button index: 0 - 7
     2. Click type: 0 - 2 (single, double, long)
  * Output:
    1. BLE event type: 0 - 1 (HID media key, letter key)
    2. Payload: 0 - 255 (HID media key, letter key)
2. [blenvs] Receives a read event (0x11)
   1. Button 1: 1
   2. Single click: 1
3. [blenvs] Combines keys into 1 & 1 into 11 (string)
4. [config] Retrieves value for key 11 from the key/value store
