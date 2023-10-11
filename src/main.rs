#![no_main]
#![no_std]

extern crate log;
extern crate receiver;
extern crate tokio;
extern crate keyboard;
extern crate esp_idf_hal;

use esp_idf_sys as _;
use log::{error, info};
use tokio::time::{sleep, Duration};
use keyboard::Keyboard;
// use config::CONFIG;

#[no_mangle]
#[tokio::main(flavor = "multi_thread")]
async fn app_main() {
  esp_idf_sys::link_patches();
  esp_idf_svc::log::EspLogger::initialize_default();

  // WDT OFF
  // unsafe {
  //   esp_idf_sys::esp_task_wdt_delete(esp_idf_sys::xTaskGetIdleTaskHandleForCPU(
  //     esp_idf_hal::cpu::core() as u32,
  //   ));
  // };

  // info!("Starting receiver");
  // match receiver::begin().await {
  //   Ok(_) => info!("Receiver ended"),
  //   Err(e) => error!("Receiver error: {:?}", e),
  // }

  let mut keyboard = Keyboard::new();

  loop {
    if !keyboard.connected() {
      info!("Nothing connected ...");
      info!("Wait for 5 seconds");
      sleep(Duration::from_secs(5)).await;
      continue;
    }

    info!("Sending media key");
    keyboard.send_media_key(0x02);
    info!("Wait for 5 seconds");
    sleep(Duration::from_secs(5)).await;
  }


  // let mut config = CONFIG.get().unwrap();
  // config.set(0, 2);

  // Restart device in 5 seconds (async)
  // info!("Restarting device in 5 seconds...");
  // sleep(Duration::from_secs(5)).await;
  // info!("Restarting device...");
  // unsafe {
  //   esp_idf_sys::esp_restart();
  // };
}
