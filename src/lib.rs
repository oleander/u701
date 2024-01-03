// mod keyboard;
// mod ffi;

// use std::sync::mpsc::{channel, Receiver, Sender};
// use log::{debug, info, error};
// use lazy_static::lazy_static;
// use anyhow::{bail, Result};
// use keyboard::Keyboard;
// use std::sync::Mutex;
// use machine::Action;

// lazy_static! {
//   static ref CHANNEL: (Sender<u8>, Mutex<Receiver<u8>>) = {
//     let (send, recv) = channel();
//     let recv = Mutex::new(recv);
//     (send, recv)
//   };
// }

// async fn runtime(mut keyboard: Keyboard) -> Result<()> {
//   info!("[main] Starting main loop");

//   let mut state = machine::State::default();

//   let receiver = CHANNEL.1.lock().unwrap();
//   info!("[main] Entering loop, waiting for events");
//   while let Ok(event_id) = receiver.recv() {
//     match state.transition(event_id) {
//       Some(Action::Media(_keys)) => keyboard.write("M"),
//       Some(Action::Short(_index)) => keyboard.write("S"),
//       None => debug!("[main] No action {}", event_id)
//     }
//   }

//   bail!("[main] Event loop ended");
// }

// pub fn on_event(event: Option<&[u8; 4]>) {
//   match event {
//     Some(&[_, _, 0, _]) => debug!("Button was released"),
//     Some(&[_, _, n, _]) => CHANNEL.0.send(n).unwrap(),
//     None => error!("[on_event] [BUG] Received {:?} event", event)
//   }
// }

#![no_main]
#![feature(never_type)]
#![allow(unused_imports)]

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

const SERVICE_UUID: BleUuid = Uuid16(0x1812);
const CHAR_UUID: BleUuid = Uuid16(0x2A4D);
const BLE_BUTTONS_NAME: &str = "tob";

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

    let device = scan
      .active_scan(true)
      .interval(490)
      .window(450)
      .find_device(i32::MAX, |device| device.is_advertising_service(&SERVICE_UUID))
      .await
      .expect("Failed to find device")
      .expect("No device found");

    let mut client = BLEClient::new();

    // client.on_passkey_request(callback)
    client.on_connect(move |client| {
      info!("Connected to device");
      info!("Updating connection parameters");
      client.update_conn_params(120, 120, 0, 60);
    });

    client.on_disconnect(move |_thing| {
      warn!("Disconnected from device");
      unsafe { esp_idf_sys::esp_restart(); };
    });

    info!("Connecting to device");
    client.connect(device.addr()).await.expect("Failed to connect to device");
    info!("Securing connection");
    client.secure_connection().await.expect("Failed to secure connection");
    info!("Connection secured");

    info!("Getting service");
    let service = client.get_service(SERVICE_UUID).await.expect("Failed to get service");
    info!("Getting characteristic");
    let characteristic = service.get_characteristic(CHAR_UUID).await.expect("Failed to get characteristic");

    info!("Subscribing to notifications");
    let status = characteristic
      .on_notify(move |_data| {
        info!("Received notification from device");
        let mut keyboard = keyboard.lock();
        if keyboard.connected() {
          keyboard.write("Hello world");
        }
      })
      .subscribe_notify(false)
      .await;

    match status {
      Ok(_) => info!("Subscribed to notifications"),
      Err(e) => warn!("Failed to subscribe to notifications: {:?}", e)
    }

    loop {
      Timer::after(Duration::from_secs(1)).await;
    }
  });
}
