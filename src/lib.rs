mod ffi;
mode keyboard;

use std::sync::mpsc::{channel, Receiver, Sender};
use log::{debug, info};
use lazy_static::lazy_static;
use anyhow::{bail, Result};
use std::sync::Mutex;
use machine::Action;
use ffi::*;
use keyboard::*;

lazy_static! {
  static ref CHANNEL: (Sender<u8>, Mutex<Receiver<u8>>) = {
    let (send, recv) = channel();
    let recv = Mutex::new(recv);
    (send, recv)
  };
}

fn main() -> Result<()> {
  info!("[main] Starting main loop");

  let receiver = CHANNEL.1.lock().unwrap();
  let mut state = machine::State::default();
  let mut keyboard = Keyboard::new();

  info!("[main] Entering loop, waiting for events");
  while let Ok(event_id) = receiver.recv() {
    match state.transition(event_id) {
      Some(Action::Media(keys)) => keyboard.write("media");
      Some(Action::Short(index)) => keyboard.write("short");
      None => debug!("[main] No action {}", event_id)
    }
  }

  bail!("[main] Event loop ended");
}

pub fn on_event(event: Option<&[u8; 4]>) {
  match event {
    Some(&[_, _, 0, _]) => debug!("Button was released"),
    Some(&[_, _, n, _]) => CHANNEL.0.send(n).unwrap(),
    // None => error!("[on_event] [BUG] Received {:?} event", event)
    None => CHANNEL.0.send(0x50).unwrap()
  }
}
