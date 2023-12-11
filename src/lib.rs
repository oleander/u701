#![allow(clippy::missing_safety_doc)]
#![feature(assert_matches)]
#![allow(dead_code)]

extern crate lazy_static;
extern crate env_logger;
extern crate anyhow;
extern crate libc;
extern crate log;

use crossbeam_channel::{unbounded, Receiver, Sender};
use lazy_static::lazy_static;
use log::{error, info, warn};
use machine::{Data, State};
use std::sync::Mutex;

lazy_static! {
  static ref STATE: Mutex<State> = Mutex::new(State::default());
  static ref CHANNEL: (Sender<u8>, Receiver<u8>) = unbounded();
}

extern "C" {
  fn restart(reason: *const libc::wchar_t);
  fn ble_keyboard_is_connected() -> bool;
  fn ble_keyboard_print(xs: *const u8);
  fn ble_keyboard_write(xs: *const u8);
  fn configure_ota();
  fn sleep(ms: u32);
}

#[no_mangle]
pub extern "C" fn setup_rust() {
  env_logger::builder().filter(None, log::LevelFilter::Debug).init();
  info!("Setup rust");
}

#[no_mangle]
pub fn on_event(event_id: u8) {
  match event_id {
    0 => info!("Button {} released", event_id),
    n => CHANNEL.0.send(n).unwrap()
  };
}

#[no_mangle]
pub extern "C" fn process_ble_events() {
  let Some(data) = CHANNEL.1.try_recv().ok() else {
    return;
  };

  info!("Received event id {}", data);
  let mut state = STATE.lock().unwrap();
  match state.event(data) {
    Some(Data::Media(keys)) => send_media_key(keys),
    Some(Data::Short(index)) => send_shortcut(index),
    None => warn!("No event to send event id {:?}", data)
  };
}

fn send_media_key(keys: [u8; 2]) {
  info!("[media] Sending media key {:?}", keys);
  unsafe { ble_keyboard_write(keys.as_ptr()) };
}

fn send_shortcut(index: u8) {
  info!("[shortcut] Sending shortcut at index {}", index);
  unsafe { ble_keyboard_print([b'a' + index - 1].as_ptr()) };
}

#[no_mangle]
pub unsafe extern "C" fn c_on_event(event: *const u8, len: usize) {
  match std::slice::from_raw_parts(event, len).get(..len) {
    Some(&[_, _, n, _]) => on_event(n),
    Some(something) => error!("[BUG] Unexpected event {:?}", something),
    None => error!("[BUG] Unexpected event size, got {} (expected {})", len, 4)
  }
}
