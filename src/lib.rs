#![allow(unused_imports)]
#![feature(never_type)]
#![allow(dead_code)]
#![no_main]

use std::sync::mpsc::{sync_channel, RecvTimeoutError};
use lazy_static::lazy_static;
use log::{debug, error};
use machine::Action;

extern crate esp_idf_hal;
extern crate lazy_static;
extern crate log;

mod keyboard;

use std::sync::Arc;
use embassy_time::{Duration, Timer};
use esp32_nimble::utilities::mutex::Mutex;
use esp32_nimble::utilities::BleUuid;
use esp32_nimble::utilities::BleUuid::Uuid16;
use esp32_nimble::{BLEClient, BLEDevice};
use esp_idf_hal::task::block_on;
use keyboard::Keyboard;
use log::{info, warn};
use tokio::sync::Notify;

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
    let mut keyboard = Keyboard::new();
    let notify = Arc::new(Notify::new());
    let notify_clone = notify.clone();
    let (tx, rx) = sync_channel(3);

    keyboard.on_authentication_complete(move |conn| {
      info!("Connected to {:?}", conn);
      info!("Notifying notify");
      notify_clone.notify_one();
    });

    info!("Waiting for notify to be notified");
    notify.notified().await;

    let device = BLEDevice::take();
    let scan = device.get_scan();

    let device = scan
      .active_scan(true)
      .interval(100)
      .window(99)
      .find_device(i32::MAX, |device| device.is_advertising_service(&SERVICE_UUID))
      .await
      .expect("Failed to find device")
      .expect("No device found");

    let mut client = BLEClient::new();

    client.on_connect(move |client| {
      info!("Updating connection parameters");
      client.update_conn_params(120, 120, 0, 60).unwrap();
    });

    client.on_disconnect(move |_thing| {
      warn!("Disconnected from device");
      unsafe {
        esp_idf_sys::esp_restart();
      };
    });

    info!("Connecting to device");
    client.connect(device.addr()).await.expect("Failed to connect to device");
    info!("Securing connection");
    client.secure_connection().await.expect("Failed to secure connection");
    info!("Connection secured");

    info!("Waiting for connection to be established");
    Timer::after(Duration::from_millis(150)).await;

    'done: for service in client.get_services().await.unwrap() {
      if service.uuid() != SERVICE_UUID {
        continue;
      }

      for characteristic in service.get_characteristics().await.unwrap() {
        if characteristic.uuid() != CHAR_UUID || !characteristic.can_notify() {
          continue;
        }

        let sender = tx.clone();
        characteristic.on_notify(move |event| {
          info!("Received notification from device: {:?}", event);

          match event {
            [_, _, 0, _] => debug!("Button was released"),
            [_, _, n, _] => sender.send(*n).unwrap(),
            otherwise => {
              error!("[on_event] [BUG] Received {:?} event", otherwise)
            }
          }
        });

        match characteristic.subscribe_notify(false).await {
          Ok(_) => {
            info!("Subscribed to notifications!");
            break 'done;
          },
          Err(e) => {
            warn!("Failed to subscribe to notifications: {:?}", e);
            warn!("Will continue to try to subscribe to notifications");
          }
        }
      }
    }

    info!("Done!");

    let mut state = machine::State::default();
    let timeout = std::time::Duration::from_millis(50);

    unsafe {
      esp_idf_sys::esp_task_wdt_init(50 * 2, true);
      esp_idf_sys::esp_task_wdt_add(esp_idf_sys::xTaskGetCurrentTaskHandle());
    }

    info!("Entering loop, waiting for events");

    loop {
      match rx.recv_timeout(timeout) {
        Ok(event_id) => {
          info!("[main] Received event {}", event_id);

          unsafe {
            esp_idf_sys::esp_task_wdt_reset();
          };

          match state.transition(event_id) {
            Some(Action::Media(keys)) => keyboard.send_media_key(keys).await,
            Some(Action::Short(index)) => keyboard.send_shortcut(index).await,
            None => info!("[main] No action {}", event_id)
          }

          unsafe {
            esp_idf_sys::esp_task_wdt_reset();
          };
        },

        Err(RecvTimeoutError::Timeout) => unsafe {
          unsafe {
            esp_idf_sys::esp_task_wdt_reset();
          };
        },

        Err(RecvTimeoutError::Disconnected) => {
          error!("Disconnected from channel, will restart");
          unsafe {
            esp_idf_sys::esp_restart();
          }
        }
      }
    }
  });
}

// use anyhow::{bail, Result};
// use keyboard::Keyboard;
// use std::sync::Mutex;
// use machine::Action;

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
