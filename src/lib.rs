#![no_std]
#![allow(dead_code)]
#![allow(clippy::missing_safety_doc)]

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
