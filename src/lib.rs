#![no_std]

extern crate linked_list_allocator;
extern crate lazy_static;
extern crate hashbrown;
extern crate anyhow;
extern crate alloc;
extern crate spin;
extern crate log;

mod keyboard;
mod constants;

use core::option::Option::{None, Some};
use crate::keyboard::Keyboard;
use crate::constants::*;
use anyhow::*;
use log::*;



#[derive(Clone, Debug, Copy)]
struct MediaControlKey(u8, u8);

impl From<MediaControlKey> for [u8; 2] {
  fn from(key: MediaControlKey) -> Self {
    [key.0, key.1]
  }
}

#[derive(Debug)]
enum InvalidButtonTransitionError {
  InvalidButton(InputState, InputState)
}


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

use ButtonIdentifier::*;



impl InputState {
  // use Meta::*;
  // use Button::*;

  fn from(id: u8) -> Option<Self> {
    use InputState::*;
    use MetaButton::*;

    match id {
      1 => Some(Meta(M1)),
      2 => Some(Regular(A2)),
      3 => Some(Regular(A3)),
      4 => Some(Regular(A4)),
      5 => Some(Meta(M2)),
      6 => Some(Regular(B2)),
      7 => Some(Regular(B3)),
      8 => Some(Regular(B4)),
      _ => None
    }
  }

  fn transition_to(&self, next: InputState) -> Result<(Option<BluetoothEvent>, InputState), InvalidButtonTransitionError> {
    use InvalidButtonTransitionError::*;
    use MetaButton::*;
    use InputState::*;

    let event = match (self, next) {
      // [INVALID] Meta -> Meta
      (from @ Meta(_), to @ Meta(_)) => return Err(InvalidButton(from.clone(), to)),

      // [OK] Meta 1 -> Regular
      (Meta(M1), Regular(button)) => META_BUTTON_EVENTS_ONE.get(&button),

      // [OK] Meta 2 -> Regular
      (Meta(M2), Regular(button)) => META_BUTTON_EVENTS_TWO.get(&button),

      // [OK] Regular -> Regular
      (_, Regular(button)) => REGULAR_BUTTON_EVENTS.get(&button),

      // [OK] Regular -> Meta
      (_, Meta(_)) => None,

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

  match event {
    Some(BluetoothEvent::MediaControlKey(key)) => {
      KEYBOARD.lock().send_media_key(key.into());
    },
    Some(BluetoothEvent::Letter(letter)) => {
      KEYBOARD.lock().send_char(letter);
    },
    None => {
      warn!("No event for button click: {:?}", curr_state);
    }
  }
}
