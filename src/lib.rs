mod ffi;

use tokio::sync::mpsc::{UnboundedReceiver, UnboundedSender};
use lazy_static::lazy_static;
use log::{error, info, warn};
use machine::Data;
use std::sync::Mutex;
use anyhow::{bail, Result};
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
      Some(Data::Write(xs)) => ble_write(xs),
      Some(Data::Print(n)) => ble_print(n),
      Some(Data::Reset) => ble_reset(),
      None => warn!("No data was produced")
    }
  }

  bail!("Event loop ended");
}

fn ble_write(xs: [u8; 2]) {
  info!("Writing {:?}", xs);
  unsafe {
    ble_keyboard_write(xs.as_ptr());
  }
}

fn ble_print(index: u8) {
  info!("Printing {}", index);
  unsafe { ble_keyboard_print([b'a' + index - 1].as_ptr()) };
}

fn ble_reset() {
  info!("Resetting");
  unsafe {
    ble_keyboard_reset();
  }
}

pub fn on_event(event: Option<&[u8; 4]>) {
  match event {
    Some(&[_, _, 0, _]) => warn!("Button was released, ignoring"),
    Some(&[_, _, n, _]) => CHANNEL.0.send(n).unwrap(),
    None => error!("Nothing was received")
  }
}
