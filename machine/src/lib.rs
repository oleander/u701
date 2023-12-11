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
  Meta(u8),
  Float
}

#[derive(PartialEq, Debug)]
pub enum Data {
  Media([u8; 2]),
  Short(u8)
}

impl State {
  // Translates key idn into key press states
  fn transition(mut self, next: u8) -> State {
    self = match (self, next) {
      // Any -> Meta: ignore previous
      (_, meta_id @ (M1 | M2)) => State::Meta(meta_id),

      // Meta -> Regular: run shortcut
      (State::Meta(meta_id), event_id) => State::Combo(meta_id, event_id),

      // Regular -> Regular: run key
      (_, media_id) => State::Key(media_id)
    };

    self.clone()
  }

  // Converts key presses into payloads
  pub fn event(&self, next: u8) -> Option<Data> {
    match self.transition(next) {
      // Meta -> Regular: run shortcut
      State::Combo(meta_id, event_id) => META.get(&meta_id).and_then(|meta| meta.get(&event_id).map(|id| Data::Short(*id))),
      // Regular: run key
      State::Key(event_id) => EVENT.get(&event_id).map(|&a| Data::Media(a)),
      // Meta: wait for next press
      State::Meta(_) => None,
      State::Float => None
    }
  }
}

impl Default for State {
  fn default() -> Self {
    State::Float
  }
}

#[cfg(test)]
mod tests {
  use std::assert_matches::assert_matches;
  use super::*;
  use super::constants::buttons::{A2, B2, M1, M2};

  #[test]
  fn test_new() {
    let state = State::default();
    assert_eq!(state, State::Float);
  }

  #[test]
  fn test_default() {
    let state = State::default();
    assert_eq!(state, State::Float);
  }

  #[test]
  fn test_transition() {
    let  state = State::default();

    // Test transition from any to Meta
    state.transition(M1);
    assert_eq!(state, State::Meta(M1));

    // Test transition from Meta to Regular
    state.transition(A2);
    assert_eq!(state, State::Combo(M1, A2));

    // Test transition from Regular to Regular
    state.transition(B2);
    assert_eq!(state, State::Key(B2));
  }

  #[test]
  fn test_event() {
    let mut state = State::default();

    // Test Meta -> Regular
    assert_eq!(state.event(M1), None);
    assert_matches!(state.event(A2), Some(Data::Short(_)));

    // Test Regular -> Regular
    assert_matches!(state.event(B2), Some(Data::Media(_)));

    // Test Meta -> Regular
    assert_eq!(state.event(M2), None);
    assert_matches!(state.event(A2), Some(Data::Short(_)));
  }
}
