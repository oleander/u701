#![no_std]
#![feature(alloc_error_handler)]

extern crate log;
extern crate lazy_static;
extern crate anyhow;
extern crate spin;
extern crate hashbrown;
extern crate alloc;

use log::*;
use spin::Mutex;
use lazy_static::*;
use anyhow::*;
use hashbrown::HashMap;
use core::option::Option::{None, Some};
use core::result::Result::Ok;

extern crate linked_list_allocator;
use linked_list_allocator::LockedHeap;

#[global_allocator]
static GLOBAL_ALLOCATOR: LockedHeap = LockedHeap::empty();

#[alloc_error_handler]
fn on_alloc_error(layout: alloc::alloc::Layout) -> ! {
  panic!("allocation error: {:?}", layout)
}

#[panic_handler]
fn on_panic(_info: &core::panic::PanicInfo) -> ! {
  loop {}
}

#[derive(Clone, Debug, Copy)]
struct MediaControlKey(u8, u8);

const VOLUME_DOWN_KEY: MediaControlKey = MediaControlKey(64, 0);
const NEXT_TRACK: MediaControlKey = MediaControlKey(1, 0);
const PREV_TRACK: MediaControlKey = MediaControlKey(2, 0);
const PLAY_PAUSE: MediaControlKey = MediaControlKey(8, 0);
const VOLUME_UP: MediaControlKey = MediaControlKey(32, 0);
const EJECT: MediaControlKey = MediaControlKey(16, 0);

#[derive(Eq, Hash, PartialEq, Clone, Debug, Copy)]
enum ButtonIdentifier {
  A2,
  A3,
  A4,
  B2,
  B3,
  B4
}

#[derive(Clone, Debug, Copy)]
enum MetaButton {
  M1,
  M2
}

#[derive(Clone, Debug, Copy)]
enum InputState {
  Meta(MetaButton),
  Regular(ButtonIdentifier),
  Undefined
}

#[derive(Clone, Debug, Copy)]
enum BluetoothEvent {
  MediaControlKey(MediaControlKey),
  Letter(u8)
}

lazy_static! {
  // Button 2-4 & 6-8
   static ref REGULAR_BUTTON_EVENTS: HashMap<ButtonIdentifier, BluetoothEvent> = {
    let mut table = HashMap::new();
    table.insert(ButtonIdentifier::A2, BluetoothEvent::MediaControlKey(VOLUME_DOWN_KEY));
    table.insert(ButtonIdentifier::A3, BluetoothEvent::MediaControlKey(PREV_TRACK));
    table.insert(ButtonIdentifier::A4, BluetoothEvent::MediaControlKey(PLAY_PAUSE));
    table.insert(ButtonIdentifier::B2, BluetoothEvent::MediaControlKey(VOLUME_UP));
    table.insert(ButtonIdentifier::B3, BluetoothEvent::MediaControlKey(NEXT_TRACK));
    table.insert(ButtonIdentifier::B4, BluetoothEvent::MediaControlKey(EJECT));
    table
  };

  // Button 1
   static ref META_BUTTON_EVENTS_ONE: HashMap<ButtonIdentifier, BluetoothEvent> = {
    let mut table = HashMap::new();
    table.insert(ButtonIdentifier::A2, BluetoothEvent::Letter(2));
    table.insert(ButtonIdentifier::A3, BluetoothEvent::Letter(3));
    table.insert(ButtonIdentifier::A4, BluetoothEvent::Letter(4));
    table.insert(ButtonIdentifier::B2, BluetoothEvent::Letter(6));
    table.insert(ButtonIdentifier::B3, BluetoothEvent::Letter(7));
    table.insert(ButtonIdentifier::B4, BluetoothEvent::Letter(8));
    table
  };

  // Button 6
  static ref META_BUTTON_EVENTS_TWO: HashMap<ButtonIdentifier, BluetoothEvent> = {
    let mut table = HashMap::new();
    table.insert(ButtonIdentifier::A2, BluetoothEvent::Letter(10));
    table.insert(ButtonIdentifier::A3, BluetoothEvent::Letter(11));
    table.insert(ButtonIdentifier::A4, BluetoothEvent::Letter(12));
    table.insert(ButtonIdentifier::B2, BluetoothEvent::Letter(14));
    table.insert(ButtonIdentifier::B3, BluetoothEvent::Letter(15));
    table.insert(ButtonIdentifier::B4, BluetoothEvent::Letter(16));
    table
  };

  static ref CURRENT_INPUT_STATE: Mutex<InputState> = Mutex::new(InputState::Undefined);
}

fn send_bluetooth_event(event: Option<BluetoothEvent>) {
  // todo!("Send event: {:?}", event);
}

#[derive(Debug)]
enum InvalidButtonTransitionError {
  InvalidButton(InputState, InputState)
}

impl InputState {
  // use Meta::*;
  // use Button::*;

  fn from(id: u8) -> Option<Self> {
    match id {
      1 => Some(InputState::Meta(MetaButton::M1)),
      2 => Some(InputState::Regular(ButtonIdentifier::A2)),
      3 => Some(InputState::Regular(ButtonIdentifier::A3)),
      4 => Some(InputState::Regular(ButtonIdentifier::A4)),
      5 => Some(InputState::Meta(MetaButton::M2)),
      6 => Some(InputState::Regular(ButtonIdentifier::B2)),
      7 => Some(InputState::Regular(ButtonIdentifier::B3)),
      8 => Some(InputState::Regular(ButtonIdentifier::B4)),
      _ => None
    }
  }

  fn transition_to(&self, next: InputState) -> Result<(Option<BluetoothEvent>, InputState), InvalidButtonTransitionError> {
    use MetaButton::*;
    use InputState::*;
    use InvalidButtonTransitionError::*;

    let event = match (self, next) {
      // [INVALID] Meta -> Meta
      (from @ Meta(_), to @ Meta(_)) => return Err(InvalidButton(from.clone(), to)),

      // [OK] Meta 1 -> Regular
      (Meta(M1), Regular(button)) => META_BUTTON_EVENTS_ONE.get(&button),

      // [OK] Meta 2 -> Regular
      (Meta(M2), Regular(button)) => META_BUTTON_EVENTS_TWO.get(&button),

      // [OK] Regular -> Meta
      (_, Meta(_)) => None,

      // [OK] Regular -> Regular
      (_, Regular(button)) => REGULAR_BUTTON_EVENTS.get(&button),

      // [BUG] ?? -> Undefined
      (_, Undefined) => {
        panic!("[BUG] Cannot transition to undefined state")
      }
    };

    Ok((event.cloned(), next))
  }
}

#[no_mangle]
fn handle_button_click(index: u8) {
  let curr_state = match InputState::from(index) {
    Some(state) => state,
    None => return error!("Invalid button index: {}", index)
  };

  let mut state_guard = CURRENT_INPUT_STATE.lock();
  let (event, new_state) = match state_guard.transition_to(curr_state) {
    Ok(result) => result,
    Err(e) => return error!("Invalid button transition: {:?}", e)
  };

  *state_guard = new_state;
  send_bluetooth_event(event);
}
