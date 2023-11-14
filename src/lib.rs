#![no_std]
#![allow(dead_code)]
#![allow(clippy::missing_safety_doc)]

extern crate log;
use log::*;

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
  loop {}
}

#[no_mangle]
fn r#loop() -> ! {
  loop {}
}

#[no_mangle]
fn setup() -> ! {
  loop {}
}

#[no_mangle]
fn handle_click(index: u8) {
  info!("handle_click: {}", index);
}
