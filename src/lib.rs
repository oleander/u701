#![no_std]

extern crate log;
use log::{error, info};
mod event;
use event::{ID, State};

// expose event to c++
extern "C" {
  fn event(id: ID, state: State);
}

#[no_mangle]
extern "C" fn hello() {
  info!("Hello from rust!");
}

