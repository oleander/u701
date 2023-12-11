use tokio::sync::mpsc::{UnboundedReceiver, UnboundedSender};
use lazy_static::lazy_static;
use log::{error, info, warn};
use machine::{Data, State};
use std::sync::Mutex;
use anyhow::{bail, Result};

#[no_mangle]
pub unsafe extern "C" fn c_on_event(event: *const u8, len: usize) {
  crate::on_event(std::slice::from_raw_parts(event, len).try_into().ok());
}

extern "C" {
  pub fn ble_keyboard_is_connected() -> bool;
  pub fn ble_keyboard_print(xs: *const u8);
  pub fn ble_keyboard_write(xs: *const u8);
  pub fn sleep(ms: u32);
  pub fn init_arduino();
}

#[no_mangle]
#[tokio::main]
pub async extern "C" fn app_main() -> i32 {
  env_logger::builder().filter(None, log::LevelFilter::Debug).init();

  info!("[app_main] Calling setup");
  unsafe {
    init_arduino();
  }

  info!("[app_main] Entering main loop");
  if let Err(e) = crate::main().await {
    error!("[error] Error: {:?}", e);
    return 1;
  }

  return 0;
}
