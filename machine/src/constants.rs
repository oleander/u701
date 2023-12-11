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
    meta1.insert(A2, 48); // '1' + 48 = 'a'
    meta1.insert(A3, 49); // '1' + 49 = 'b'
    meta1.insert(A4, 50); // '1' + 50 = 'c'
    meta1.insert(B2, 51); // '1' + 51 = 'd'
    meta1.insert(B3, 52); // '1' + 52 = 'e'
    meta1.insert(B4, 53); // '1' + 53 = 'f'

    let mut meta2 = HashMap::new();
    meta2.insert(A2, 0); // '1' + 0 = '1'
    meta2.insert(A3, 1); // '1' + 1 = '2'
    meta2.insert(A4, 2); // '1' + 2 = '3'
    meta2.insert(B2, 3); // '1' + 3 = '4'
    meta2.insert(B3, 4); // '1' + 4 = '5'
    meta2.insert(B4, 5); // '1' + 5 = '6'

    let mut table = HashMap::new();
    table.insert(M1, meta1);
    table.insert(M2, meta2);
    table
  };
}

// meta 2
// a, d
// b, e
// c, f

// meta 1
// f, i
// g, e
// h, k

// kjfedfedfjikjkfedkiii

// 2: defabc
// 1: ijkfgh
