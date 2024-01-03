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
#[tokio::main(flavor = "current_thread")]
pub async extern "C" fn app_main() -> i32 {
  env_logger::builder().filter(None, log::LevelFilter::Info).init();

  info!("Setup keyboard");
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

  info!("Wait a bit");
  esp_idf_hal::delay::Ets::delay_ms(150);


  unsafe {
    init_arduino();
  }

  if let Err(e) = crate::runtime(keyboard).await {
    error!("[error] Error: {:?}", e);
  }

  return 0;
}
