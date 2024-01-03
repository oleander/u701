mod ffi;
mod keyboard;

use std::sync::mpsc::{channel, Receiver, Sender};
use log::{debug, info, error};
use lazy_static::lazy_static;
use anyhow::{bail, Result};
use std::sync::Mutex;
use machine::Action;
use keyboard::Keyboard;
use ffi::*;

lazy_static! {
  static ref CHANNEL: (Sender<u8>, Mutex<Receiver<u8>>) = {
    let (send, recv) = channel();
    let recv = Mutex::new(recv);
    (send, recv)
  };
}

use tokio::sync::Notify;


fn main() -> Result<()> {
  info!("[main] Starting main loop");

  let receiver = CHANNEL.1.lock().unwrap();
  let mut keyboard = Keyboard::new();
  let mut state = machine::State::default();

  let notify = Arc::new(Notify::new());
  let notify_clone = notify.clone();

  keyboard.on_authentication_complete(move |conn| {
    info!("Connected to {:?}", conn);
    info!("Notifying notify");
    notify_clone.notify_one();
  });

  info!("Waiting for notify to be notified");
  notify.notified().await;

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
