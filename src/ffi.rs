use log::{error, info};

#[no_mangle]
pub unsafe extern "C" fn c_on_event(event: *const u8, len: usize) {
  crate::on_event(std::slice::from_raw_parts(event, len).try_into().ok());
}

extern "C" {
  pub fn ble_keyboard_print(xs: *const u8);
  pub fn ble_keyboard_write(xs: *const u8);
  pub fn init_arduino();
}

#[no_mangle]
#[tokio::main(flavor = "current_thread")]
pub async extern "C" fn app_main() -> i32 {
  env_logger::builder().filter(None, log::LevelFilter::Info).init();

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

pub fn send_media_key(keys: [u8; 2]) {
  info!("[media] Sending media key {:?}", keys);
  unsafe { ble_keyboard_write(keys.as_ptr()) };
}

pub fn send_shortcut(index: u8) {
  info!("[shortcut] Sending shortcut at index {}", index);
  unsafe { ble_keyboard_print([b'a' + index, 0].as_ptr()) };
}
