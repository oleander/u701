mod ffi;

use std::sync::mpsc::{channel, Receiver, Sender};
use log::{debug, error, info};
use lazy_static::lazy_static;
use anyhow::{bail, Result};
use std::sync::Mutex;
use machine::Action;
use ffi::*;

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

  info!("[main] Entering loop, waiting for events");
  loop {
    match receiver.recv() {
      Ok(event_id) => {
        match state.transition(event_id) {
          Some(Action::Media(keys)) => send_media_key(keys),
          Some(Action::Short(index)) => send_shortcut(index),
          None => debug!("[main] No action {}", event_id)
        }
      },
      Err(_) => break
    }
  }

  bail!("[main] Event loop ended");
}

pub fn on_event(event: Option<&[u8; 4]>) {
  match event {
    Some(&[_, _, 0, _]) => debug!("Button was released"),
    Some(&[_, _, n, _]) => CHANNEL.0.send(n).unwrap(),
    None => error!("[on_event] [BUG] Nothing received")
  }
}
