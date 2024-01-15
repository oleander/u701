use std::collections::HashMap;
use lazy_static::*;
use buttons::*;

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum MediaControl {
  VolumeDown,
  NextTrack,
  PrevTrack,
  PlayPause,
  VolumeUp,
  Eject
}

impl From<MediaControl> for [u8; 2] {
  fn from(control: MediaControl) -> [u8; 2] {
    match control {
      MediaControl::VolumeDown => [64, 0],
      MediaControl::NextTrack => [1, 0],
      MediaControl::PrevTrack => [2, 0],
      MediaControl::PlayPause => [8, 0],
      MediaControl::VolumeUp => [32, 0],
      MediaControl::Eject => [16, 0]
    }
  }
}

pub mod buttons {
  pub const M1: u8 = 0x04; // Corresponds to BUTTON_1: Red (Meta)
  pub const A2: u8 = 0x50; // Corresponds to BUTTON_2: Black (Volume down)
  pub const A3: u8 = 0x51; // Corresponds to BUTTON_3: Blue (Prev track)
  pub const A4: u8 = 0x52; // Corresponds to BUTTON_4: Black (Play/Pause)
  pub const M2: u8 = 0x29; // Corresponds to BUTTON_5: Red (Meta)
  pub const B2: u8 = 0x4F; // Corresponds to BUTTON_6: Black (Volume up)
  pub const B3: u8 = 0x05; // Corresponds to BUTTON_7: Blue (Next track)
  pub const B4: u8 = 0x28; // Corresponds to BUTTON_8: Black (Toggle AC)
}

lazy_static! {
  pub static ref EVENT: HashMap<u8, MediaControl> = {
    let mut table = HashMap::new();
    table.insert(A2, MediaControl::VolumeDown);
    table.insert(A3, MediaControl::PrevTrack);
    table.insert(A4, MediaControl::PlayPause);
    table.insert(B2, MediaControl::VolumeUp);
    table.insert(B3, MediaControl::NextTrack);
    table.insert(B4, MediaControl::Eject);
    table
  };

  pub static ref META: HashMap<u8, HashMap<u8, u8>> = {
    let mut meta1 = HashMap::new();
    meta1.insert(A2, 0);
    meta1.insert(A3, 1);
    meta1.insert(A4, 2);
    meta1.insert(B2, 3);
    meta1.insert(B3, 4);
    meta1.insert(B4, 5);

    let mut meta2 = HashMap::new();
    meta2.insert(A2, 6);
    meta2.insert(A3, 7);
    meta2.insert(A4, 8);
    meta2.insert(B2, 9);
    meta2.insert(B3, 10);
    meta2.insert(B4, 11);

    let mut table = HashMap::new();
    table.insert(M1, meta1);
    table.insert(M2, meta2);
    table
  };
}
