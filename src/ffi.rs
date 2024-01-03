use esp32_nimble::{enums::{SecurityIOCap, PowerType, PowerLevel, AuthReq}, BLEDevice, BLEClient};
use esp32_nimble::utilities::BleUuid;
use esp32_nimble::utilities::BleUuid::Uuid16;
use log::{error, info, debug};
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

const SERVICE_UUID: BleUuid = Uuid16(0x1812);
const CHAR_UUID: BleUuid = Uuid16(0x2A4D);

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

  debug!("[characteristic] finding service ...");
  let mut characteristic = client
    .get_service(SERVICE_UUID)
    .await
    .unwrap()
    .get_characteristics()
    .await
    .unwrap()
    .find(|x| x.uuid() == CHAR_UUID && x.can_notify())
    .unwrap()
    .to_owned();

  debug!("[characteristic] subscribing to notifications ...");
  characteristic.on_notify(move |data| {
    info!("[on_notify] data={:?}", data);

    if let [0, 0, id, _] = data {
      crate::on_event(Some(&[0, 0, *id, 0]));
    } else {
      error!("[on_notify] invalid data: {:?}", data);
    }
  });

  if let Err(e) = characteristic.subscribe_notify(false).await {
    error!("[characteristic] could not subscribe to notifications: {:?}", e);
  }


  if let Err(e) = crate::runtime(keyboard).await {
    error!("[error] Error: {:?}", e);
  }

  return 0;
}
