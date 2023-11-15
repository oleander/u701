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
mod types;
mod impls;

use core::option::Option::{None, Some};
use crate::keyboard::Keyboard;
use core::result::Result::Ok;
use crate::constants::*;
use ButtonIdentifier::*;
use crate::types::*;
use anyhow::*;
use log::*;

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
