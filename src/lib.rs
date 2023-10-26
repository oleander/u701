#![allow(dead_code)]
#![allow(clippy::missing_safety_doc)]
#![feature(assert_matches)]

extern crate hashbrown;
extern crate lazy_static;
extern crate anyhow;

use thingbuf::mpsc::{self, Receiver, Sender};
use lazy_static::lazy_static;
use hashbrown::HashMap;
use std::sync::Mutex;
use anyhow::{anyhow, bail, Result};

#[derive(PartialEq, Debug, Copy, Clone)]
#[repr(C)]
pub struct MediaKeyReport(u8, u8);
pub type ID = u8;

// The BLE event generated by the button
#[derive(Debug, PartialEq, Clone)]
pub enum BLEEvent {
  MediaKey(MediaKeyReport),
  Letter(u8)
}

impl Default for BLEEvent {
  fn default() -> Self {
    BLEEvent::Letter(0)
  }
}
// The click event generated by the button
// Press event: [0, 0, ID, 0]
// Release event: [0, 0, 0, 0]
pub type ClickEvent = [u8; 4];

// If the button was released or pressed
#[derive(Copy, Clone, Debug, PartialEq)]
pub enum PushState {
  Down(ID),
  Up(ID)
}

const KEY_MEDIA_VOLUME_DOWN: MediaKeyReport = MediaKeyReport(64, 0);
const KEY_MEDIA_NEXT_TRACK: MediaKeyReport = MediaKeyReport(1, 0);
const KEY_MEDIA_PREV_TRACK: MediaKeyReport = MediaKeyReport(2, 0);
const KEY_MEDIA_PLAY_PAUSE: MediaKeyReport = MediaKeyReport(8, 0);
const KEY_MEDIA_VOLUME_UP: MediaKeyReport = MediaKeyReport(32, 0);
const KEY_MEDIA_EJECT: MediaKeyReport = MediaKeyReport(16, 0);

const KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION: MediaKeyReport = MediaKeyReport(0, 64);
const KEY_MEDIA_LOCAL_MACHINE_BROWSER: MediaKeyReport = MediaKeyReport(0, 1);
const KEY_MEDIA_EMAIL_READER: MediaKeyReport = MediaKeyReport(0, 128);
const KEY_MEDIA_WWW_BOOKMARKS: MediaKeyReport = MediaKeyReport(0, 4);
const KEY_MEDIA_WWW_HOME: MediaKeyReport = MediaKeyReport(128, 0);
const KEY_MEDIA_CALCULATOR: MediaKeyReport = MediaKeyReport(0, 2);
const KEY_MEDIA_WWW_SEARCH: MediaKeyReport = MediaKeyReport(0, 8);
const KEY_MEDIA_WWW_STOP: MediaKeyReport = MediaKeyReport(0, 16);
const KEY_MEDIA_WWW_BACK: MediaKeyReport = MediaKeyReport(0, 32);
const KEY_MEDIA_STOP: MediaKeyReport = MediaKeyReport(4, 0);

pub const BUTTON_1: u8 = 0x04; // Red (Meta)
pub const BUTTON_2: u8 = 0x50; // Black (Volume down)
pub const BUTTON_3: u8 = 0x51; // Blue (Prev track)
pub const BUTTON_4: u8 = 0x52; // Black (Play/Pause)
pub const BUTTON_5: u8 = 0x29; // Red (Meta)
pub const BUTTON_6: u8 = 0x4F; // Black (Volume up)
pub const BUTTON_7: u8 = 0x05; // Blue (Next track)
pub const BUTTON_8: u8 = 0x28; // Black (Toggle AC)

// Meta keys, much like the shift key on a regular keyboard
pub const META_1: u8 = BUTTON_1;
pub const META_2: u8 = BUTTON_5;

lazy_static! {
  // Button 2-4 & 6-8
   static ref REGULAR_LOOKUP: HashMap<u8, BLEEvent> = {
    let mut table = HashMap::new();
    // BUTTON_1 is a meta key
    table.insert(BUTTON_2, BLEEvent::MediaKey(KEY_MEDIA_VOLUME_DOWN));
    table.insert(BUTTON_3, BLEEvent::MediaKey(KEY_MEDIA_PREV_TRACK));
    table.insert(BUTTON_4, BLEEvent::MediaKey(KEY_MEDIA_PLAY_PAUSE));
    // BUTTON_5 is a meta key
    table.insert(BUTTON_6, BLEEvent::MediaKey(KEY_MEDIA_VOLUME_UP));
    table.insert(BUTTON_7, BLEEvent::MediaKey(KEY_MEDIA_NEXT_TRACK));
    table.insert(BUTTON_8, BLEEvent::MediaKey(KEY_MEDIA_EJECT));
    table
  };

  // Button 1
   static ref META_LOOKUP_1: HashMap<u8, BLEEvent> = {
    let mut table = HashMap::new();
    // table.insert(BUTTON_1, BLEEvent::Letter(1));
    table.insert(BUTTON_2, BLEEvent::Letter(2));
    table.insert(BUTTON_3, BLEEvent::Letter(3));
    table.insert(BUTTON_4, BLEEvent::Letter(4));
    // table.insert(BUTTON_5, BLEEvent::Letter(5));
    table.insert(BUTTON_6, BLEEvent::Letter(6));
    table.insert(BUTTON_7, BLEEvent::Letter(7));
    table.insert(BUTTON_8, BLEEvent::Letter(8));
    table
  };

  // Button 6
  static ref META_LOOKUP_2: HashMap<u8, BLEEvent> = {
    let mut table = HashMap::new();
    // BUTTON_1 is a meta key
    // table.insert(BUTTON_1, BLEEvent::Letter(9));
    table.insert(BUTTON_2, BLEEvent::Letter(10));
    table.insert(BUTTON_3, BLEEvent::Letter(11));
    table.insert(BUTTON_4, BLEEvent::Letter(12));
    // table.insert(BUTTON_5, BLEEvent::Letter(13));
    table.insert(BUTTON_6, BLEEvent::Letter(14));
    table.insert(BUTTON_7, BLEEvent::Letter(15));
    table.insert(BUTTON_8, BLEEvent::Letter(16));
    table
  };

  static ref ACTIVE_STATE: Mutex<PushState> = Mutex::new(PushState::Up(0));
  static ref BLE_EVENT_QUEUE: (Sender<BLEEvent>, Receiver<BLEEvent>) = mpsc::channel(10);
}

impl PushState {
  // Allow us to keep track of the state of the button
  // @event The the click event (4 bytes)
  // @return 1. The new state of the button
  //         2. The event to send to the host
  //            1. MediaKeyReport (i.e Play/Pause)
  //            2. Letter (i.e. 'a')
  //            3. None (i.e. no event)
  pub fn transition(&self, event: &ClickEvent) -> (Self, Option<BLEEvent>) {
    use PushState::*;

    let next_state = match (event, *self) {
      // The button was released after being pressed
      // [Ok] Pressed -> Released (updated)
      ([.., 0, _], Down(id)) => Up(id),
      // A button has already been pushed
      // [User error] Pressed -> Pressed (ignored)
      ([.., _, _], Down(id)) => Down(id),

      // The button was released after being released
      // [Bug] Released -> Released (ignored)
      ([.., 0, _], Up(id)) => Up(id),

      // The button was pressed after being released
      // [Ok] Released -> Pressed (updated)
      ([.., id, _], Up(_)) => Down(*id)
    };

    let next_event = match (self, next_state) {
      // Meta key was pressed together with another key
      (Up(META_1), Down(id)) => META_LOOKUP_1.get(&id),

      // Meta key was pressed together with another key
      (Up(META_2), Down(id)) => META_LOOKUP_2.get(&id),

      // A regular key was pressed
      (_, Down(id)) => REGULAR_LOOKUP.get(&id),

      // A regular key was released
      (_, Up(_)) => None
    };

    (next_state, next_event.cloned())
  }
}

extern "C" {
  fn ble_keyboard_print(xs: *const u8);
  fn ble_keyboard_write(xs: *const u8);
  fn ble_keyboard_is_connected() -> bool;
}

#[no_mangle]
pub extern "C" fn setup_rust() {
  println!("Setup rust");
}

#[no_mangle]
pub unsafe extern "C" fn transition_from_cpp(event: *const u8, len: usize) {
  println!("Received event from C++");

  let event_slice: &[u8] = unsafe { std::slice::from_raw_parts(event, len) };
  let mut click_event = [0u8; 4];

  match len.cmp(&4) {
    std::cmp::Ordering::Less => {
      return println!("[BUG] Event too short, abort");
    },
    std::cmp::Ordering::Greater => {
      println!("Event too long, truncating");
      click_event.copy_from_slice(event_slice[0..4].as_ref());
    },
    std::cmp::Ordering::Equal => {
      click_event.copy_from_slice(event_slice);
    }
  }

  if let Err(e) = transition(&click_event) {
    println!("Failed to transition: {:?}", e);
  }
}

#[no_mangle]
pub extern "C" fn process_ble_events() {
  match BLE_EVENT_QUEUE.1.try_recv() {
    Ok(BLEEvent::MediaKey(report)) => {
      println!("Sending media key report: {:?}", report);
      let xs: [u8; 2] = [report.0, report.1];
      unsafe { ble_keyboard_write(xs.as_ptr()) };
    },
    Ok(BLEEvent::Letter(index)) => {
      println!("Sending letter: {:?}", index);
      let printable_char = format!("{}", (b'a' + index - 1) as char);
      unsafe { ble_keyboard_print(printable_char.as_str().as_ptr()) };
    },
    Err(e) => {
      eprintln!("Failed to receive event: {:?}", e);
    }
  }
}

fn transition(curr_event: &ClickEvent) -> Result<()> {
  println!("Received event: {:?}", curr_event);

  let Ok(mut active_state) = ACTIVE_STATE.lock() else {
    bail!("Failed to lock mutex");
  };

  let (next_state, next_event) = active_state.transition(curr_event);
  *active_state = next_state;

  if let Some(event) = next_event {
    BLE_EVENT_QUEUE.0.try_send(event).map_err(|e| anyhow!(e))?;
  }

  Ok(())
}

