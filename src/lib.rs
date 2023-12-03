#![allow(dead_code)]
#![allow(clippy::missing_safety_doc)]
#![feature(assert_matches)]

extern crate lazy_static;
extern crate env_logger;
extern crate anyhow;
extern crate log;

use thingbuf::mpsc::{StaticChannel, StaticReceiver, StaticSender};
use thingbuf::mpsc::errors::{TryRecvError, TrySendError};
use log::{debug, error, info, warn};
use std::collections::HashMap;
use lazy_static::lazy_static;
use anyhow::{bail, Result};
use std::sync::Mutex;

#[derive(PartialEq, Debug, Copy, Clone)]
#[repr(C)]
pub struct MediaKey(u8, u8);

impl MediaKey {
  pub fn reverse(&self) -> [u8; 2] {
    [self.0, self.1]
  }
}

pub type ID = u8;

// The BLE event generated by the button
#[derive(Debug, PartialEq, Clone)]
pub enum BLEEvent {
  MediaKey(MediaKey),
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

const CLICK_EVENT_SIZE: usize = 4;

const KEY_MEDIA_VOLUME_DOWN: MediaKey = MediaKey(64, 0);
const KEY_MEDIA_NEXT_TRACK: MediaKey = MediaKey(1, 0);
const KEY_MEDIA_PREV_TRACK: MediaKey = MediaKey(2, 0);
const KEY_MEDIA_PLAY_PAUSE: MediaKey = MediaKey(8, 0);
const KEY_MEDIA_VOLUME_UP: MediaKey = MediaKey(32, 0);
const KEY_MEDIA_EJECT: MediaKey = MediaKey(16, 0);

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

  static ref STATIC_CHANNEL: StaticChannel<BLEEvent, 4> = StaticChannel::new();
  static ref BLE_EVENT_QUEUE: (StaticSender<BLEEvent>, StaticReceiver<BLEEvent>) = STATIC_CHANNEL.split();
  static ref ACTIVE_STATE: Mutex<PushState> = Mutex::new(PushState::Up(0));
}

impl PushState {
  // Allow us to keep track of the state of the button
  // @event The the click event (4 bytes)
  // @return 1. The new state of the button
  //         2. The event to send to the host
  //            1. MediaKey (i.e Play/Pause)
  //            2. Letter (i.e. 'a')
  //            3. None (i.e. no event)
  pub fn transition(&self, event: &ClickEvent) -> (Self, Option<BLEEvent>) {
    use PushState::*;

    info!("Transitioning from {:?} with {:?}", self, event);
    let next_state = match (event, *self) {
      // The button was released after being pressed
      // [Ok] Pressed -> Released (updated)
      ([.., 0, _], Down(id)) => Up(id),
      // A button has already been pushed
      // [User error] Pressed -> Pressed (ignored)
      ([.., _, _], prev @ Down(_)) => prev,

      // The button was released after being released
      // [Bug] Released -> Released (ignored)
      ([.., 0, _], prev @ Up(_)) => prev,

      // [Ignore] Meta button is being double pressed
      ([.., META_1 | META_2, _], prev @ Up(META_1 | META_2)) => prev,

      // The button was pressed after being released
      // [Ok] Released -> Pressed (updated)
      ([.., id, _], Up(_)) => Down(*id)
    };

    let next_event = match (self /* curr_state */, next_state) {
      (Up(META_1 | META_2), Down(META_1 | META_2)) => None,

      // Meta key was pressed together with another key
      (Up(META_1), Down(id)) => META_LOOKUP_1.get(&id),

      // Meta key was pressed together with another key
      (Up(META_2), Down(id)) => META_LOOKUP_2.get(&id),

      // A regular key was pressed
      (Up(_), Down(id)) => REGULAR_LOOKUP.get(&id),

      // A second button was pressed while the first was still pressed
      (Down(_), Down(_)) => None,

      // A regular key was released
      (_, Up(_)) => None
    };

    debug!("Next state: {:?}", next_state);
    debug!("Next event: {:?}", next_event);

    (next_state, next_event.cloned())
  }
}

extern "C" {
  fn ble_keyboard_print(xs: *const u8);
  fn ble_keyboard_write(xs: *const u8);
  fn ble_keyboard_is_connected() -> bool;
  fn sleep(ms: u32);
}

#[no_mangle]
pub extern "C" fn setup_rust() {
  env_logger::builder().filter(None, log::LevelFilter::Debug).init();
  info!("Setup rust");
}

#[no_mangle]
pub unsafe extern "C" fn handle_external_click_event(event: *const u8, len: usize) {
  info!("Received BLE click event");

  if len != CLICK_EVENT_SIZE {
    return error!("[BUG] Unexpected event size, got {} (expected {})", len, CLICK_EVENT_SIZE);
  }

  let event_slice = std::slice::from_raw_parts(event, len);
  let mut click_event = [0u8; CLICK_EVENT_SIZE];
  click_event.copy_from_slice(event_slice);

  if let Err(e) = handle_click_event(&click_event) {
    warn!("Failed to transition: {:?}", e);
  }
}

#[no_mangle]
pub extern "C" fn process_ble_events() {
  match BLE_EVENT_QUEUE.1.try_recv() {
    Ok(BLEEvent::MediaKey(report)) => {
      info!("Sending media key report: {:?}", report);
      let reversed_report = report.reverse();
      unsafe { ble_keyboard_write(reversed_report.as_ptr()) };
    },
    Ok(BLEEvent::Letter(index)) => {
      info!("Sending letter: {:?}", index);
      let letter = (b'a' + index - 1) as char;
      unsafe { ble_keyboard_print(&letter as *const _ as *const u8) };
    },
    Err(TryRecvError::Closed) => {
      error!("[BUG] Event queue is closed");
      panic!("[BUG] Event queue is closed");
    },
    Err(e) => {
      error!("Failed to receive event: {:?}", e)
    }
  }
}

fn handle_click_event(curr_event: &ClickEvent) -> Result<()> {
  info!("Received event: {:?}", curr_event);

  let mut state_guard = match ACTIVE_STATE.lock() {
    Ok(guard) => guard,
    Err(err) => bail!("Failed to lock mutex {:?}", err)
  };

  let (next_state, next_event) = state_guard.transition(curr_event);
  *state_guard = next_state;

  if let Some(event) = next_event {
    match BLE_EVENT_QUEUE.0.try_send(event) {
      Err(TrySendError::Closed(_)) => {
        error!("[BUG] Event queue is closed");
        bail!("[BUG] Event queue is closed");
      },
      Err(TrySendError::Full(_)) => {
        error!("[BUG] Event queue is full");
        bail!("[BUG] Event queue is full")
      },
      Err(unkown) => {
        error!("[BUG] Unknown error: {:?}", unkown);
        bail!("[BUG] Unknown error: {:?}", unkown)
      },
      Ok(_) => debug!("Event sent successfully")
    }
  }

  Ok(())
}
