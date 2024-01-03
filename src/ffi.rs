use log::{error, info};
use crate::Keyboard;
use std::sync::Arc;
use tokio::sync::Notify;

#[no_mangle]
pub unsafe extern "C" fn c_on_event(event: *const u8, len: usize) {
  crate::on_event(std::slice::from_raw_parts(event, len).try_into().ok());
}

extern "C" {
  pub fn init_arduino();
}

#[no_mangle]
pub extern "C" fn app_main() -> i32 {
  env_logger::builder().filter(None, log::LevelFilter::Info).init();

  info!("[app_main] Calling setup");

  unsafe {
    init_arduino();
  }

  info!("[app_main] Entering main loop");
  tokio::spawn(async move {
    let mut keyboard = Keyboard::new();

    let notify = Arc::new(Notify::new());
    let notify_clone = notify.clone();

    keyboard.on_authentication_complete(move |conn| {
      info!("Connected to {:?}", conn);
      info!("Notifying notify");
      notify_clone.notify_one();
    });

    info!("Waiting for notify to be notified");
    notify.notified().await;

    if let Err(e) = crate::runtime(keyboard).await {
      error!("[error] Error: {:?}", e);
    }
  });

  return 0;
}
