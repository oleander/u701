#![allow(clippy::missing_safety_doc)]
#![feature(assert_matches)]
#![allow(dead_code)]

extern crate lazy_static;
extern crate env_logger;
extern crate anyhow;
extern crate libc;
extern crate log;

use lazy_static::lazy_static;
use log::{debug, error, info, warn};
use machine::{Data, State};
use tokio::sync::{mpsc::{UnboundedSender, UnboundedReceiver}};
use std::sync::RwLock;
use std::sync::Mutex;

lazy_static! {
  static ref STATE: Mutex<State> = Mutex::new(State::default());
}

lazy_static! {
  static ref CHANNEL: (UnboundedSender<u8>, Mutex<UnboundedReceiver<u8>>) = {
      let (send, mut rev) = tokio::sync::mpsc::unbounded_channel();
      let mut rev = Mutex::new(rev);
      (send, rev)
  };
}

use anyhow::{Result, Context};

async fn main() -> Result<()> {
  let mut state = machine::State::default();
  while let Some(event_id) = CHANNEL.1.lock().unwrap().recv().await {
    match state.event(event_id) {
      Some(Data::Media(keys)) => send_media_key(keys),
      Some(Data::Short(index)) => send_shortcut(index),
      None => warn!("No event to send event id {:?}", event_id)
    };
  };

  Ok(())
}

extern "C" {
  fn c_unwind(reason: *const libc::wchar_t);
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
unsafe extern "C" fn on_event(event_id: u8) {
  match event_id {
    0 => info!("Button {} released", event_id),
    n => CHANNEL.0.send(n).unwrap()
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

#[no_mangle]
pub unsafe extern "C" fn c_tick() {
}

fn unwind(reason: &str) -> ! {
  error!("{}", reason);
  unsafe { c_unwind(reason.as_ptr() as *const libc::wchar_t) };
  unreachable!()
}

#[no_mangle]
#[tokio::main]
async extern "C" fn app_main() -> usize {
  env_logger::init();
  info!("[main] Starting main");
  if let Err(e) = main().await {
    error!("[app_main] Error: {:?}", e);
    return 1;
  }

  return 0;
}
