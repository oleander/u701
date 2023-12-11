use std::collections::HashMap;
use lazy_static::*;

pub mod media {
  pub const VOLUME_DOWN: [u8; 2] = [64, 0];
  pub const NEXT_TRACK: [u8; 2] = [1, 0];
  pub const PREV_TRACK: [u8; 2] = [2, 0];
  pub const PLAY_PAUSE: [u8; 2] = [8, 0];
  pub const VOLUME_UP: [u8; 2] = [32, 0];
  pub const EJECT: [u8; 2] = [16, 0];
}

pub mod buttons {
  pub const RELEASE: u8 = 0x00;
  pub const M1: u8 = 0x04; // Corresponds to BUTTON_1: Red (Meta)
  pub const A2: u8 = 0x50; // Corresponds to BUTTON_2: Black (Volume down)
  pub const A3: u8 = 0x51; // Corresponds to BUTTON_3: Blue (Prev track)
  pub const A4: u8 = 0x52; // Corresponds to BUTTON_4: Black (Play/Pause)
  pub const M2: u8 = 0x29; // Corresponds to BUTTON_5: Red (Meta)
  pub const B2: u8 = 0x4F; // Corresponds to BUTTON_6: Black (Volume up)
  pub const B3: u8 = 0x05; // Corresponds to BUTTON_7: Blue (Next track)
  pub const B4: u8 = 0x28; // Corresponds to BUTTON_8: Black (Toggle AC)
}

use buttons::*;
use media::*;

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
  pub static ref META: HashMap<u8, u8> = {
    let mut table = HashMap::new();
    table.insert(M1 | A2, 1);
    table.insert(M1 | A3, 2);
    table.insert(M1 | A4, 3);

    table.insert(M1 | B2, 4);
    table.insert(M1 | B3, 5);
    table.insert(M1 | B4, 6);

    table.insert(M2 | A2, 7);
    table.insert(M2 | A3, 8);
    table.insert(M2 | A4, 9);

    table.insert(M2 | B2, 10);
    table.insert(M2 | B3, 11);
    table.insert(M2 | B4, 12);
    table
  };
}
