mod ffi;

extern crate lazy_static;
extern crate env_logger;
extern crate anyhow;
extern crate libc;
extern crate log;

use tokio::sync::mpsc::{UnboundedReceiver, UnboundedSender};
use lazy_static::lazy_static;
use log::{error, info, warn};
use machine::Data;
use std::sync::Mutex;
use anyhow::Result;
use anyhow::bail;
use ffi::*;

lazy_static! {
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

pub fn on_event(event: Option<&[u8; 4]>) {
  match event {
    Some(&[_, _, 0, _]) => warn!("Button was released, ignoring"),
    Some(&[_, _, n, _]) => CHANNEL.0.send(n).unwrap(),
    None => error!("Nothing was received"),
  }
}

