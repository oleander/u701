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

pub struct State {
  curr: Event
}

#[derive(PartialEq, Debug)]
pub enum Data {
  Media([u8; 2]),
  Short(u8)
}

impl State {
  pub fn new(event: Event) -> Self {
    Self {
      curr: event
    }
  }

  // Translates key idn into key press states
  fn transition(&mut self, next: u8) -> Event {
    self.curr = match (self.curr, next) {
      // Any -> Meta: ignore previous
      (_, meta_id @ (M1 | M2)) => Event::Meta(meta_id),

      // Meta -> Regular: run shortcut
      (Event::Meta(meta_id), event_id) => Event::Duo(meta_id, event_id),

      // Regular -> Regular: run key
      (_, media_id) => Event::Key(media_id)
    };

    self.curr.clone()
  }

  // Converts key presses into payloads
  pub fn event(&mut self, next: u8) -> Option<Data> {
    match self.transition(next) {
      // Meta -> Regular: run shortcut
      Event::Duo(meta_id, event_id) => META.get(&meta_id).and_then(|meta| meta.get(&event_id).map(|id| Data::Short(*id))),
      // Regular: run key
      Event::Key(event_id) => EVENT.get(&event_id).map(|&a| Data::Media(a)),
      // Meta: wait for next press
      Event::Meta(_) => None,
      Event::Float => None
    }
  }
}

impl Default for State {
  fn default() -> Self {
    Self::new(Event::Float)
  }
}

#[cfg(test)]
mod tests {
  use std::assert_matches::assert_matches;
  use super::*;
  use super::constants::buttons::{A2, B2, M1, M2};

  #[test]
  fn test_new() {
    let state = State::new(Event::Float);
    assert_eq!(state.curr, Event::Float);
  }

  #[test]
  fn test_default() {
    let state = State::default();
    assert_eq!(state.curr, Event::Float);
  }

  #[test]
  fn test_transition() {
    let mut state = State::new(Event::Float);

    // Test transition from any to Meta
    state.transition(M1);
    assert_eq!(state.curr, Event::Meta(M1));

    // Test transition from Meta to Regular
    state.transition(A2);
    assert_eq!(state.curr, Event::Duo(M1, A2));

    // Test transition from Regular to Regular
    state.transition(B2);
    assert_eq!(state.curr, Event::Key(B2));
  }

  #[test]
  fn test_event() {
    let mut state = State::new(Event::Float);

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
