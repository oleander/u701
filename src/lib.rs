mod keyboard;
mod ffi;

use std::sync::mpsc::{channel, Receiver, Sender};
use log::{debug, info, error};
use lazy_static::lazy_static;
use anyhow::{bail, Result};
use keyboard::Keyboard;
use std::sync::Mutex;
use machine::Action;

lazy_static! {
  static ref CHANNEL: (Sender<u8>, Mutex<Receiver<u8>>) = {
    let (send, recv) = channel();
    let recv = Mutex::new(recv);
    (send, recv)
  };
}

async fn runtime(mut keyboard: Keyboard) -> Result<()> {
  info!("[main] Starting main loop");

  let mut state = machine::State::default();


  let receiver = CHANNEL.1.lock().unwrap();
  info!("[main] Entering loop, waiting for events");
  while let Ok(event_id) = receiver.recv() {
    match state.transition(event_id) {
      Some(Action::Media(_keys)) => keyboard.write("M"),
      Some(Action::Short(_index)) => keyboard.write("S"),
      None => debug!("[main] No action {}", event_id)
    }
  }

  bail!("[main] Event loop ended");
}

pub fn on_event(event: Option<&[u8; 4]>) {
  match event {
    Some(&[_, _, 0, _]) => debug!("Button was released"),
    Some(&[_, _, n, _]) => CHANNEL.0.send(n).unwrap(),
    None => error!("[on_event] [BUG] Received {:?} event", event)
  }
}
