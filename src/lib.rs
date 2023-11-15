#![no_std]

extern crate log;
extern crate lazy_static;
extern crate anyhow;
extern crate hashbrown;

use log::*;
use lazy_static::*;
use anyhow::*;
use hashbrown::HashMap;
use core::option::Option::{Some, None};

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
  loop {}
}

const KEY_MEDIA_VOLUME_DOWN: MediaKey = MediaKey(64, 0);
const KEY_MEDIA_NEXT_TRACK: MediaKey = MediaKey(1, 0);
const KEY_MEDIA_PREV_TRACK: MediaKey = MediaKey(2, 0);
const KEY_MEDIA_PLAY_PAUSE: MediaKey = MediaKey(8, 0);
const KEY_MEDIA_VOLUME_UP: MediaKey = MediaKey(32, 0);
const KEY_MEDIA_EJECT: MediaKey = MediaKey(16, 0);

enum Button {
  A2,
  A3,
  A4,
  B2,
  B3,
  B4
}

enum Meta {
  M1,
  M2
}

enum State {
  Meta(Meta),
  Regular(Button),
  Undefined
}

enum BLEEvent {
  MediaKey(MediaKey),
  Letter(u8)
}

lazy_static! {
  // Button 2-4 & 6-8
   static ref REGULAR_LOOKUP: HashMap<Button, BLEEvent> = {
    let mut table = HashMap::new();
    table.insert(Button::A2, BLEEvent::MediaKey(KEY_MEDIA_VOLUME_DOWN));
    table.insert(Button::A3, BLEEvent::MediaKey(KEY_MEDIA_PREV_TRACK));
    table.insert(Button::A4, BLEEvent::MediaKey(KEY_MEDIA_PLAY_PAUSE));
    table.insert(Button::B2, BLEEvent::MediaKey(KEY_MEDIA_VOLUME_UP));
    table.insert(Button::B3, BLEEvent::MediaKey(KEY_MEDIA_NEXT_TRACK));
    table.insert(Button::B4, BLEEvent::MediaKey(KEY_MEDIA_EJECT));
    table
  };

  // Button 1
   static ref META_LOOKUP_1: HashMap<Button, BLEEvent> = {
    let mut table = HashMap::new();
    table.insert(Button::A2, BLEEvent::Letter(2));
    table.insert(Button::A3, BLEEvent::Letter(3));
    table.insert(Button::A4, BLEEvent::Letter(4));
    table.insert(Button::B2, BLEEvent::Letter(6));
    table.insert(Button::B3, BLEEvent::Letter(7));
    table.insert(Button::B4, BLEEvent::Letter(8));
    table
  };

  // Button 6
  static ref META_LOOKUP_2: HashMap<u8, BLEEvent> = {
    let mut table = HashMap::new();
    table.insert(Button::A2, BLEEvent::Letter(10));
    table.insert(Button::A3, BLEEvent::Letter(11));
    table.insert(Button::A4, BLEEvent::Letter(12));
    table.insert(Button::B2, BLEEvent::Letter(14));
    table.insert(Button::B3, BLEEvent::Letter(15));
    table.insert(Button::B4, BLEEvent::Letter(16));
    table
  };

  static ref STATE: Mutex<Option<State>> = Mutex::new(Undefined);
}

fn send(event: Optional<BLEEvent>) {
  // todo!("Send event: {:?}", event);
}


impl State {
  use Meta::*;
  use Button::*;

  fn from(id: u8) -> Option<Self> {
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

  fn transition_to(self, next: State) -> Result<Optional<BLEEvent>, String> {
    let event = match (self, next) {
      // [INVALID] Meta -> Meta
      (Meta(_), Meta(_)) => {
        bail!("Meta button pressed twice in a row");
      },

      // [OK] Meta 1 -> Regular
      (Meta(M1), Regular(button)) => {
        META_LOOKUP_1.get(&button)
      },

      // [OK] Meta 2 -> Regular
      (Meta(M2), Regular(button)) => {
        META_LOOKUP_2.get(&button)
      }

      // [OK] Regular -> Meta
      (_, Meta(_)) => {
        None
      },

      // [OK] Regular -> Regular
      (_, button) => {
        REGULAR_LOOKUP.get(&button)
      }
    };

    self = next;

    Ok(event)
  }
}

// impl std::fmt::Display for Button {
//   fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
//     match self {
//       Button::Meta1 => write!(f, "A1 [Red] (Meta)"),
//       Button::A2 => write!(f, "A2 [Black] (Volume down)"),
//       Button::A3 => write!(f, "A3 [Blue] (Prev track)"),
//       Button::A4 => write!(f, "A4 [Black] (Play/Pause)"),
//       Button::Meta2 => write!(f, "B1 [Red] (Meta)"),
//       Button::B2 => write!(f, "B2 [Black] (Volume up)"),
//       Button::B3 => write!(f, "B3 [Blue] (Next track)"),
//       Button::B4 => write!(f, "B4 [Black] (Toggle AC)")
//     }
//   }
// }

// enum 

  // curr_button.transition();
  // let Some(prev_button) = PREV_BUTTON_PRESS.lock().unwrap() else {
  //   return REGULAR_LOOKUP.get(&curr_button);
  // };

  // let event = match (prev_button, curr_button) {
  //   (Button::Meta1 | Button::Meta2, Button::Meta1 | Button::Meta2) => {
  //     bail!("Meta button pressed twice in a row");
  //   },
  //   (Button::Meta1, button) => META_LOOKUP_1.get(&button),
  //   (Button::Meta2, button) => META_LOOKUP_2.get(&button),
  //   (_, button) => REGULAR_LOOKUP.get(&button)
  // };



#[no_mangle]
fn handle_click(index: u8) {
  let Some(curr_state) = State::from(index) else {
    return error!("Invalid button: {}", index);
  };

  match STATE.lock().unwrap().transition_to(curr_state) {
    Ok(event) => send(event),
    Err(e) => error!("Error: {}", e)
  }
}
