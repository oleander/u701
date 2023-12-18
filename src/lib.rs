mod ffi;

use tokio::sync::mpsc::{unbounded_channel, UnboundedReceiver, UnboundedSender};
use log::{error, info, warn, debug};
use lazy_static::lazy_static;
use anyhow::{bail, Result};
use tokio::sync::Mutex;
use machine::Action;
use ffi::*;

lazy_static! {
  static ref CHANNEL: (UnboundedSender<u8>, Mutex<UnboundedReceiver<u8>>) = {
    let (send, rev) = unbounded_channel();
    let rev = Mutex::new(rev);
    (send, rev)
  };
}

// Runtime processor for the incoming BLE events
async fn main() -> Result<()> {
  info!("[main] Starting main loop");

  let mut receiver = CHANNEL.1.lock().await;
  let mut state = machine::State::default();

  info!("[main] Enterting loop, waiting for events");
  while let Some(event_id) = receiver.recv().await {
    match state.transition(event_id) {
      Some(Action::Media(keys)) => send_media_key(keys),
      Some(Action::Short(index)) => send_shortcut(index),
      None => warn!("[main] No event id {}", event_id)
    };
  }

  bail!("[main] Event loop ended");
}

// BLE button callback given the button id being pressed or 0 if released
// Example: [0, 0, 50, 0] -> Button 3 was pressed
// Example: [0, 0, 0, 0] -> A button was released
pub fn on_event(event: Option<&[u8; 4]>) {
  match event {
    Some(&[_, _, 0, _]) => debug!("Button was released"),
    Some(&[_, _, n, _]) => CHANNEL.0.send(n).unwrap(),
    None => error!("[BUG] Nothing was received")
  }
}
