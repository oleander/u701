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
fn panic(_info: &core::panic::PanicInfo) -> ! {
  loop {}
}

#[derive(Clone, Debug, Copy)]
struct MediaKey(u8, u8);

const KEY_MEDIA_VOLUME_DOWN: MediaKey = MediaKey(64, 0);
const KEY_MEDIA_NEXT_TRACK: MediaKey = MediaKey(1, 0);
const KEY_MEDIA_PREV_TRACK: MediaKey = MediaKey(2, 0);
const KEY_MEDIA_PLAY_PAUSE: MediaKey = MediaKey(8, 0);
const KEY_MEDIA_VOLUME_UP: MediaKey = MediaKey(32, 0);
const KEY_MEDIA_EJECT: MediaKey = MediaKey(16, 0);

#[derive(Eq, Hash, PartialEq, Clone, Debug, Copy)]
enum R {
  A2,
  A3,
  A4,
  B2,
  B3,
  B4
}

#[derive(Clone, Debug, Copy)]
enum M {
  M1,
  M2
}

#[derive(Clone, Debug, Copy)]
enum State {
  Meta(M),
  Regular(R),
  Undefined
}

#[derive(Clone, Debug, Copy)]
enum BLEEvent {
  MediaKey(MediaKey),
  Letter(u8)
}

lazy_static! {
  // Button 2-4 & 6-8
   static ref REGULAR_LOOKUP: HashMap<R, BLEEvent> = {
    let mut table = HashMap::new();
    table.insert(R::A2, BLEEvent::MediaKey(KEY_MEDIA_VOLUME_DOWN));
    table.insert(R::A3, BLEEvent::MediaKey(KEY_MEDIA_PREV_TRACK));
    table.insert(R::A4, BLEEvent::MediaKey(KEY_MEDIA_PLAY_PAUSE));
    table.insert(R::B2, BLEEvent::MediaKey(KEY_MEDIA_VOLUME_UP));
    table.insert(R::B3, BLEEvent::MediaKey(KEY_MEDIA_NEXT_TRACK));
    table.insert(R::B4, BLEEvent::MediaKey(KEY_MEDIA_EJECT));
    table
  };

  // Button 1
   static ref META_LOOKUP_1: HashMap<R, BLEEvent> = {
    let mut table = HashMap::new();
    table.insert(R::A2, BLEEvent::Letter(2));
    table.insert(R::A3, BLEEvent::Letter(3));
    table.insert(R::A4, BLEEvent::Letter(4));
    table.insert(R::B2, BLEEvent::Letter(6));
    table.insert(R::B3, BLEEvent::Letter(7));
    table.insert(R::B4, BLEEvent::Letter(8));
    table
  };

  // Button 6
  static ref META_LOOKUP_2: HashMap<R, BLEEvent> = {
    let mut table = HashMap::new();
    table.insert(R::A2, BLEEvent::Letter(10));
    table.insert(R::A3, BLEEvent::Letter(11));
    table.insert(R::A4, BLEEvent::Letter(12));
    table.insert(R::B2, BLEEvent::Letter(14));
    table.insert(R::B3, BLEEvent::Letter(15));
    table.insert(R::B4, BLEEvent::Letter(16));
    table
  };

  static ref STATE: Mutex<State> = Mutex::new(State::Undefined);
}

fn send(event: Option<BLEEvent>) {
  // todo!("Send event: {:?}", event);
}

#[derive(Debug)]
enum ButtonError {
  InvalidButton(State, State)
}

impl State {
  // use Meta::*;
  // use Button::*;

  fn from(id: u8) -> Option<Self> {
    match id {
      1 => Some(State::Meta(M::M1)),
      2 => Some(State::Regular(R::A2)),
      3 => Some(State::Regular(R::A3)),
      4 => Some(State::Regular(R::A4)),
      5 => Some(State::Meta(M::M2)),
      6 => Some(State::Regular(R::B2)),
      7 => Some(State::Regular(R::B3)),
      8 => Some(State::Regular(R::B4)),
      _ => None
    }
  }

  fn transition_to(&self, next: State) -> Result<(Option<BLEEvent>, State), ButtonError> {
    let event = match (self, next) {
      // [INVALID] Meta -> Meta
      (from @ State::Meta(_), to @ State::Meta(_)) => return Err(ButtonError::InvalidButton(from.clone(), to)),

      // [OK] Meta 1 -> Regular
      (State::Meta(M::M1), State::Regular(button)) => META_LOOKUP_1.get(&button),

      // [OK] Meta 2 -> Regular
      (State::Meta(M::M2), State::Regular(button)) => META_LOOKUP_2.get(&button),

      // [OK] Regular -> Meta
      (_, State::Meta(_)) => None,

      // [OK] Regular -> Regular
      (_, State::Regular(button)) => REGULAR_LOOKUP.get(&button),

      // [BUG] ?? -> Undefined
      (_, State::Undefined) => {
        panic!("[BUG] Cannot transition to undefined state")
      }
    };

    Ok((event.cloned(), next))
  }
}

#[no_mangle]
fn handle_click(index: u8) {
  let curr_state = match State::from(index) {
    Some(state) => state,
    None => return error!("Invalid button index: {}", index)
  };

  let mut state_guard = STATE.lock();
  let (event, new_state) = match state_guard.transition_to(curr_state) {
    Ok(result) => result,
    Err(e) => return error!("Invalid button transition: {:?}", e)
  };

  *state_guard = new_state;
  send(event);
}
