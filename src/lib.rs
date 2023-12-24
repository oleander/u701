#![no_main]
#![feature(never_type)]

extern crate esp_idf_hal;
extern crate lazy_static;
extern crate log;

mod keyboard;

use embassy_time::{Duration, Timer};
use esp32_nimble::utilities::mutex::Mutex;
use esp32_nimble::utilities::BleUuid;
use esp32_nimble::utilities::BleUuid::Uuid16;
use esp32_nimble::{BLEClient, BLEDevice};
use esp_idf_hal::task::block_on;
use keyboard::Keyboard;
use log::{info, warn};
use std::sync::Arc;

const SERVICE_UUID: BleUuid = Uuid16(0x1812);
const CHAR_UUID: BleUuid = Uuid16(0x2A4D);
const BLE_BUTTONS_NAME: &str = "key";

#[no_mangle]
fn app_main() {
  esp_idf_sys::link_patches();
  esp_idf_svc::log::EspLogger::initialize_default();
  esp_idf_svc::timer::embassy_time::driver::link();

  info!("Setup connection to BLE buttons");
  block_on(async {
    let keyboard = Mutex::new(Keyboard::new());

    let device = BLEDevice::take();
    let scan = device.get_scan();

    let device = scan.active_scan(true).interval(100).window(99).find_device(u32::MAX.try_into().unwrap(), |found_device| {
      found_device.name().starts_with(b"key")
    });

    let Ok(Some(device)) = device.await else {
      return warn!("No device found");
    };

    let mut client = BLEClient::new();

    client.connect(device.addr()).await.expect("Failed to connect to device");

    let service = client.get_service(SERVICE_UUID).await.expect("Failed to get service");
    let characteristic = service.get_characteristic(CHAR_UUID).await.expect("Failed to get characteristic");

    characteristic
      .on_notify(move |_data| {
        info!("Received notification from device");
        let mut keyboard = keyboard.lock();
        if keyboard.connected() {
          keyboard.write("Hello world");
        }
      })
      .subscribe_notify(false)
      .await
      .expect("Failed to subscribe to notifications");

    info!("Waiting for notifications ...");

    loop {
      Timer::after(Duration::from_secs(1)).await;
    }
  });
}
