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
use core::result::Result::Ok;
use crate::constants::*;
use crate::types::*;
use log::*;

extern "C" fn init() {
  esp_idf_sys::link_patches();
  esp_idf_svc::log::EspLogger::initialize_default();

  info!("Starting up...");
}

extern "C" fn rust_handle_button_click(index: u8) {
  let Some(curr_state) = InputState::from(index) else {
    return error!("Invalid button index: {}", index);
  };

  if let Err(e) = handle_button_click(curr_state) {
    error!("Error handling button click: {:?}", e);
  }
}

fn handle_button_click(curr_state: InputState) -> Result<()> {
  let mut state_guard = CURRENT_INPUT_STATE.lock();
  let (event, new_state) = state_guard.transition_to(curr_state)?;
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
  };

  Ok(())
}
