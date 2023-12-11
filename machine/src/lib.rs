#![feature(restricted_std)]
#![feature(assert_matches)]
#![allow(dead_code)]

pub mod constants;

use constants::buttons::{M1, M2};
use constants::{EVENT, META};

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum Event {
  Duo(u8, u8),
  Key(u8),
  Meta(u8),
  Float
}

#[derive(Debug, Clone, Copy)]
pub struct State {
  curr: Pos
}

#[derive(Clone, Copy, PartialEq)]
enum KeyEvent {
  Key(u8),      // For regular keys like "C"
  Modifier(u8), // For keys like "Ctrl"
  Combo(u8, u8) // For keys like "Ctrl + C"
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Pos {
  Up(KeyEvent),
  Down(KeyEvent),
}

#[derive(PartialEq, Debug, Clone, Copy)]
pub enum Data {
  Write([u8; 2]),
  Print(u8),
  Reset
}

impl State {
  pub fn new(pos: Pos) -> Self {
    Self {
      curr: pos
    }
  }

  // Translates key idn into key press states
  fn transition(&mut self, next: u8) -> Pos {
    use Pos::*;
    use KeyEvent::*;

    self.curr = match (self.curr, next) {
      // 1. Key pressed
      // 2. Modifier pressed
      // Res: Combine key (1) and modifier (2)
      (Down(Key(key_id)), meta_id @ (M1 | M2)) => {
        Down(Combo(meta_id, key_id))
      },

      // 1. Modifier pressed
      // 2. Modifier pressed again
      // Res: Keep 1 pressed
      (meta @ Down(Modifier(_)), M1 | M2) => {
        meta
      },

      // 1. Any key pressed
      // 2. Key released
      // Res: Set key (1) released
      (Down(key), 0) => {
        Up(key)
      }

      // 1. Modifier pressed
      // 2. Key pressed
      // Res: Keep key (2 + mod) pressed
      (Down(Modifier(meta_id)), key_id) => {
        Up(Combo(meta_id, key_id))
      },

      // 1. Combo key pressed
      // 2. Key is pressed
      // Res: Ignore second key press
      (prev @ Down(Combo(_, _)), _) => {
        prev
      },

      // 1. Key released
      // 2. Key released again
      // Res: Keep key (1) released
      (prev @ Up(_), 0) => prev,

      // 1. Key released
      // 2. Key pressed
      // Res: Keep (2) pressed
      (Up(_), id) => {
        Down(Key(id))
      },

      // 1. Regular key pressed
      // 2. Regular key pressed again
      // Res: Ignore second key press
      (prev @ Down(Key(_)), _) => prev
    };

    self.curr.clone()
  }

  // Converts key presses into payloads
  pub fn event(&mut self, next: u8) -> Option<Data> {
    use Pos::*;
    use KeyEvent::*;

    match self.transition(next) {
      // Modifier + Regular key pressed
      Down(Combo(meta_id, key_id)) => {
        META.get(&(meta_id | key_id)).map(|&keys| Data::Print(keys)).into()
      },

      // Regular key pressed
      Down(Key(key_id)) => {
        EVENT.get(&key_id).map(|&index| Data::Write(index)).into()
      },

      // Meta key pressed
      Down(Modifier(_)) => {
        None
      },

      // Regular key released
      Up(Key(_) | Combo(_, _)) => {
        Some(Data::Reset)
      },

      // Meta key released
      Up(Modifier(_)) => {
        None
      }
    }
  }
}

impl Default for State {
  fn default() -> Self {
    Self::new(Pos::Up(KeyEvent::Key(0)))
  }
}
