#![allow(clippy::missing_safety_doc)]
#![feature(assert_matches)]
#![allow(dead_code)]

extern crate lazy_static;
extern crate env_logger;
extern crate anyhow;
extern crate libc;
extern crate log;

use lazy_static::lazy_static;
use log::{error, info, warn};
use machine::{Data, State};
use tokio::sync::mpsc::{UnboundedReceiver, UnboundedSender};
use std::sync::Mutex;
use anyhow::Result;
use anyhow::bail;

lazy_static! {
  static ref STATE: Mutex<State> = Mutex::new(State::default());
  static ref CHANNEL: (UnboundedSender<u8>, Mutex<UnboundedReceiver<u8>>) = {
    let (send, rev) = tokio::sync::mpsc::unbounded_channel();
    let rev = Mutex::new(rev);
    (send, rev)
  };
}

async fn main() -> Result<()> {
  info!("[main] Starting main loop");

  let mut receiver = CHANNEL.1.lock().unwrap();
  let mut state = machine::State::default();

  info!("Enterting loop, waiting for events");
  while let Some(event_id) = receiver.recv().await {
    match state.event(event_id) {
      Some(Data::Media(keys)) => send_media_key(keys),
      Some(Data::Short(index)) => send_shortcut(index),
      None => warn!("No event id id for {}", event_id)
    };
  }

  bail!("Event loop ended");
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
    Some(&[_, _, n, _]) => CHANNEL.0.send(n).unwrap(),
    Some(something) => error!("[BUG] Unexpected event {:?}", something),
    None => error!("[BUG] Unexpected event size, got {} (expected {})", len, 4)
  }
}

#[no_mangle]
#[tokio::main]
async extern "C" fn app_main() -> i32 {
  env_logger::builder().filter(None, log::LevelFilter::Debug).init();
  info!("[app_main] Starting main");
  if let Err(e) = main().await {
    error!("[error] Error: {:?}", e);
    return 1;
  }

  return 0;
}

extern "C" {
  fn c_unwind(reason: *const libc::wchar_t);
  fn ble_keyboard_is_connected() -> bool;
  fn ble_keyboard_print(xs: *const u8);
  fn ble_keyboard_write(xs: *const u8);
  fn configure_ota();
  fn sleep(ms: u32);
}
