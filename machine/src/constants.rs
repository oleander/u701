use std::collections::HashMap;
use lazy_static::*;
use buttons::*;
use media::*;

pub mod media {
  pub const VOLUME_DOWN: [u8; 2] = [64, 0];
  pub const NEXT_TRACK: [u8; 2] = [1, 0];
  pub const PREV_TRACK: [u8; 2] = [2, 0];
  pub const PLAY_PAUSE: [u8; 2] = [8, 0];
  pub const VOLUME_UP: [u8; 2] = [32, 0];
  pub const EJECT: [u8; 2] = [16, 0];
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
  pub static ref EVENT: HashMap<u8, [u8; 2]> = {
    let mut table = HashMap::new();
    table.insert(A2, VOLUME_DOWN);
    table.insert(A3, PREV_TRACK);
    table.insert(A4, PLAY_PAUSE);
    table.insert(B2, VOLUME_UP);
    table.insert(B3, NEXT_TRACK);
    table.insert(B4, EJECT);
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
