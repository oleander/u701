use esp32_nimble::{enums::{SecurityIOCap, PowerType, PowerLevel, AuthReq}, BLEDevice, BLEClient};
use log::{error, info};
use crate::Keyboard;
use std::sync::Arc;
use bstr::ByteSlice;
use tokio::sync::Notify;

#[no_mangle]
pub unsafe extern "C" fn c_on_event(event: *const u8, len: usize) {
  crate::on_event(std::slice::from_raw_parts(event, len).try_into().ok());
}

extern "C" {
  pub fn setup_arduino();
  pub fn setup_ble();
  pub fn setup_app();
}

#[no_mangle]
#[tokio::main(flavor = "current_thread")]
pub async extern "C" fn app_main() -> i32 {
  env_logger::builder().filter(None, log::LevelFilter::Info).init();

  unsafe {
    setup_arduino();
    // setup_ble();
  }

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

  let device = BLEDevice::take();
  device
    .set_power(PowerType::Default, PowerLevel::P9)
    .unwrap();
  device
    .security()
    .set_auth(AuthReq::Bond)
    .set_io_cap(SecurityIOCap::NoInputNoOutput);

  let ble_scan = device.get_scan();

  let device = ble_scan
    .active_scan(true)
    .interval(100)
    .window(99)
    .find_device(i32::MAX, move |device| {
      device.name().contains_str("key") || device.name().contains_str("Terrain")
    })
    .await
    .unwrap();

  let Some(device) = device else {
    ::log::warn!("device not found");
    return 1;
  };

  let mut client = BLEClient::new();
  client.connect(device.addr()).await.unwrap();
  client.secure_connection().await.unwrap();

  info!("Connected to {:?}", device);

  if let Err(e) = crate::runtime(keyboard).await {
    error!("[error] Error: {:?}", e);
  }

  return 0;
}
