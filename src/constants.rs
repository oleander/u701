use hashbrown::HashMap;
use lazy_static::*;

lazy_static! {
  // Button 2-4 & 6-8
  static ref REGULAR_BUTTON_EVENTS: HashMap<ButtonIdentifier, BluetoothEvent> = {
    let mut table = HashMap::new();
    table.insert(A2, BluetoothEvent::MediaControlKey(VOLUME_DOWN_KEY));
    table.insert(A3, BluetoothEvent::MediaControlKey(PREV_TRACK));
    table.insert(A4, BluetoothEvent::MediaControlKey(PLAY_PAUSE));
    table.insert(B2, BluetoothEvent::MediaControlKey(VOLUME_UP));
    table.insert(B3, BluetoothEvent::MediaControlKey(NEXT_TRACK));
    table.insert(B4, BluetoothEvent::MediaControlKey(EJECT));
    table
  };

  // Button 1 (Meta 1)
  static ref META_BUTTON_EVENTS_ONE: HashMap<ButtonIdentifier, BluetoothEvent> = {
    let mut table = HashMap::new();
    table.insert(A2, BluetoothEvent::Letter(2));
    table.insert(A3, BluetoothEvent::Letter(3));
    table.insert(A4, BluetoothEvent::Letter(4));
    table.insert(B2, BluetoothEvent::Letter(6));
    table.insert(B3, BluetoothEvent::Letter(7));
    table.insert(B4, BluetoothEvent::Letter(8));
    table
  };

  // Button 5 (Meta 2)
  static ref META_BUTTON_EVENTS_TWO: HashMap<ButtonIdentifier, BluetoothEvent> = {
    let mut table = HashMap::new();
    table.insert(A2, BluetoothEvent::Letter(10));
    table.insert(A3, BluetoothEvent::Letter(11));
    table.insert(A4, BluetoothEvent::Letter(12));
    table.insert(B2, BluetoothEvent::Letter(14));
    table.insert(B3, BluetoothEvent::Letter(15));
    table.insert(B4, BluetoothEvent::Letter(16));
    table
  };

  static ref CURRENT_INPUT_STATE: Mutex<InputState> = Mutex::new(InputState::Undefined);
  static ref KEYBOARD: Mutex<Keyboard> = Mutex::new(Keyboard::new());
}
