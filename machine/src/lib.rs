#![feature(restricted_std)]
#![feature(assert_matches)]
#![allow(dead_code)]

pub mod constants;

use constants::buttons::{M1, M2};
use constants::{EVENT, META};

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum State {
  Combo(u8, u8),
  Key(u8),
  Meta(u8)
}

#[derive(PartialEq, Debug, Clone, Copy)]
pub enum Data {
  Media([u8; 2]),
  Short(u8)
}

impl State {
  // Converts key presses into payloads
  #[rustfmt::skip]
  pub fn transition(&mut self, next: u8) -> Option<Data> {
    *self = match (self.clone(), next) {
      // Meta -> Meta: ignore new
      (state @ State::Meta(_), M1 | M2) => {
        state
      }

      // Any -> Meta: ignore previous
      (_, meta_id @ (M1 | M2)) => {
        State::Meta(meta_id)
      }

      // Meta -> Regular: run shortcut
      (State::Meta(meta_id), event_id) => {
        State::Combo(meta_id, event_id)
      }

      // Regular -> Regular: run key
      (_, media_id) => {
        State::Key(media_id)
      }
    };

    match self {
      // Meta -> Regular: run shortcut
      State::Combo(meta_id, event_id) => {
        META.get(&meta_id).and_then(|m| m.get(&event_id)).map(|&a| Data::Short(a))
      }

      // Regular: run key
      State::Key(event_id) => {
        EVENT.get(&event_id).map(|&a| Data::Media(a))
      }

      // Meta: wait for next press
      State::Meta(_) => {
        None
      }
    }
  }
}

impl Default for State {
  fn default() -> Self {
    State::Key(0)
  }
}

#[cfg(test)]
mod tests {
  use super::constants::buttons::*;
  use super::constants::media::*;
  use super::*;

  #[test]
  fn test_new() {
    let state = State::default();
    assert_eq!(state, State::Key(0));
  }

  #[test]
  fn test_regular() {
    let mut state = State::default();

    assert_eq!(state.transition(A2), Some(Data::Media(VOLUME_DOWN)));
    assert_eq!(state.transition(A3), Some(Data::Media(PREV_TRACK)));
    assert_eq!(state.transition(A4), Some(Data::Media(PLAY_PAUSE)));

    assert_eq!(state.transition(B2), Some(Data::Media(VOLUME_UP)));
    assert_eq!(state.transition(B3), Some(Data::Media(NEXT_TRACK)));
    assert_eq!(state.transition(B4), Some(Data::Media(EJECT)));
  }

  #[test]
  fn meta_1_with_regular() {
    let mut state = State::default();

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A2), Some(Data::Short(48)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A3), Some(Data::Short(49)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A4), Some(Data::Short(50)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B2), Some(Data::Short(51)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B3), Some(Data::Short(52)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B4), Some(Data::Short(53)));
  }

  #[test]
  fn meta_2_with_regular() {
    let mut state = State::default();

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A2), Some(Data::Short(0)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A3), Some(Data::Short(1)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A4), Some(Data::Short(2)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B2), Some(Data::Short(3)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B3), Some(Data::Short(4)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B4), Some(Data::Short(5)));
  }

  #[test]
  fn meta_1_with_meta_2() {
    let mut state = State::default();

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A2), Some(Data::Short(48)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A3), Some(Data::Short(49)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A4), Some(Data::Short(50)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B2), Some(Data::Short(51)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B3), Some(Data::Short(52)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B4), Some(Data::Short(53)));
  }

  #[test]
  fn meta_2_with_meta_1() {
    let mut state = State::default();

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A2), Some(Data::Short(0)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A3), Some(Data::Short(1)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A4), Some(Data::Short(2)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B2), Some(Data::Short(3)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B3), Some(Data::Short(4)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B4), Some(Data::Short(5)));
  }

  #[test]
  fn meta_1_with_meta_1() {
    let mut state = State::default();

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A2), Some(Data::Short(48)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A3), Some(Data::Short(49)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A4), Some(Data::Short(50)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B2), Some(Data::Short(51)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B3), Some(Data::Short(52)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B4), Some(Data::Short(53)));
  }

  #[test]
  fn meta_2_with_meta_2() {
    let mut state = State::default();

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A2), Some(Data::Short(0)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A3), Some(Data::Short(1)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A4), Some(Data::Short(2)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B2), Some(Data::Short(3)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B3), Some(Data::Short(4)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B4), Some(Data::Short(5)));
  }

  #[test]
  fn meta_1_with_meta_1_with_regular() {
    let mut state = State::default();

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A2), Some(Data::Short(48)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A3), Some(Data::Short(49)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A4), Some(Data::Short(50)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A2), Some(Data::Short(48)));
    assert_eq!(state.transition(A3), Some(Data::Media(PREV_TRACK)));
    assert_eq!(state.transition(A4), Some(Data::Media(PLAY_PAUSE)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B2), Some(Data::Short(51)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B3), Some(Data::Short(52)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B4), Some(Data::Short(53)));
  }

  #[test]
  fn meta_2_with_meta_2_with_regular() {
    let mut state = State::default();

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A2), Some(Data::Short(0)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A3), Some(Data::Short(1)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A4), Some(Data::Short(2)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A2), Some(Data::Short(0)));
    assert_eq!(state.transition(A3), Some(Data::Media(PREV_TRACK)));
    assert_eq!(state.transition(A4), Some(Data::Media(PLAY_PAUSE)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B2), Some(Data::Short(3)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B3), Some(Data::Short(4)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B4), Some(Data::Short(5)));
  }

  #[test]
  fn meta_1_with_regular_with_meta_1() {
    let mut state = State::default();

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A2), Some(Data::Short(48)));
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A3), Some(Data::Short(49)));
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A4), Some(Data::Short(50)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A2), Some(Data::Short(48)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A3), Some(Data::Short(49)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A4), Some(Data::Short(50)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B2), Some(Data::Short(51)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B3), Some(Data::Short(52)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B4), Some(Data::Short(53)));
  }
}
