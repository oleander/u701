use std::collections::HashMap;
use lazy_static::*;

pub mod media {
  pub const VOLUME_DOWN_KEY: [u8; 2] = [64, 0];
  pub const NEXT_TRACK: [u8; 2] = [1, 0];
  pub const PREV_TRACK: [u8; 2] = [2, 0];
  pub const PLAY_PAUSE: [u8; 2] = [8, 0];
  pub const VOLUME_UP: [u8; 2] = [32, 0];
  pub const EJECT: [u8; 2] = [16, 0];
}

pub mod buttons {
  pub const M1: u8 = 0; // A0 (2)
  pub const A2: u8 = 1; // A1 (3)
  pub const A3: u8 = 2; // A2 (4)
  pub const A4: u8 = 3; // A3 (5)
  pub const M2: u8 = 7; // 7 (7)
  pub const B2: u8 = 11; // 11 (11)
  pub const B3: u8 = 11; // 15 (15)
  pub const B4: u8 = 27; // 27 (27)
}

use buttons::*;
use media::*;

lazy_static! {
  pub static ref EVENT: HashMap<u8, [u8; 2]> = {
    let mut table = HashMap::new();
    table.insert(A2, VOLUME_DOWN_KEY);
    table.insert(A3, PREV_TRACK);
    table.insert(A4, PLAY_PAUSE);
    table.insert(B2, VOLUME_UP);
    table.insert(B3, NEXT_TRACK);
    table.insert(B4, EJECT);
    table
  };
  pub static ref META: HashMap<u8, HashMap<u8, u8>> = {
    let mut meta1 = HashMap::new();
    meta1.insert(A2, 10);
    meta1.insert(A3, 11);
    meta1.insert(A4, 12);
    meta1.insert(B2, 13);
    meta1.insert(B3, 14);
    meta1.insert(B4, 15);

    let mut meta2 = HashMap::new();
    meta2.insert(A2, 0);
    meta2.insert(A3, 1);
    meta2.insert(A4, 2);
    meta2.insert(B2, 3);
    meta2.insert(B3, 4);
    meta2.insert(B4, 5);

    let mut table = HashMap::new();
    table.insert(M1, meta1);
    table.insert(M2, meta2);
    table
  };
}
