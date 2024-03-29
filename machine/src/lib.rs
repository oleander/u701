#![feature(restricted_std)]
#![feature(assert_matches)]
#![allow(dead_code)]

pub mod constants;

use constants::buttons::{M1, M2};
use constants::*;

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum State {
  Combo(u8, u8),
  Key(u8),
  Meta(u8)
}

#[derive(PartialEq, Debug, Clone, Copy)]
pub enum Action {
  Media(MediaControl),
  Short(u8)
}

impl State {
  // Converts key presses into payloads
  #[rustfmt::skip]
  pub fn transition(&mut self, next: u8) -> Option<Action> {
    use State::*;
    use Action::*;

    *self = match (*self, next) {
      // Meta -> Meta: ignore new
      (state @ Meta(_), M1 | M2) => {
        state
      }

      // Any -> Meta: ignore previous
      (_, meta_id @ (M1 | M2)) => {
        Meta(meta_id)
      }

      // Meta -> Regular: run shortcut
      (Meta(meta_id), event_id) => {
        Combo(meta_id, event_id)
      }

      // Regular -> Regular: run key
      (_, media_id) => {
        Key(media_id)
      }
    };

    match self {
      // Meta -> Regular: run shortcut
      Combo(meta_id, event_id) => {
        META.get(meta_id).and_then(|m| m.get(event_id)).map(|&a| Short(a))
      }

      // Regular: run key
      Key(event_id) => {
        EVENT.get(event_id).map(|&a| Media(a))
      }

      // Meta: wait for next press
      Meta(_) => {
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
  use super::*;

  macro_rules! test_transitions {
    ($state:expr; $($input:expr => $expected:expr),+) => {
      {
        let mut state = $state;
        $(
          assert_eq!(
            state.transition($input),
            $expected,
            "Failed on input {:?}: expected {:?}, got {:?}", $input, $expected, state.transition($input)
          );
        )*
    }
    };
    ($setup:expr; $state:expr; $($input:expr => $expected:expr),+; $cleanup:expr) => {
        {
            $setup;
            let mut state = $state;
            $(
                assert_eq!(state.transition($input), $expected,
                    "Failed on input {:?}: expected {:?}, got {:?}", $input, $expected, state.transition($input));
            )*
            $cleanup;
        }
    };
}

  #[test]
  fn test_new() {
    let state = State::default();
    assert_eq!(state, State::Key(0));
  }

  #[test]
  fn test_regular() {
    test_transitions!(State::default();
      A2 => Some(Action::Media(MediaControl::VolumeDown)),
      A3 => Some(Action::Media(MediaControl::PrevTrack)),
      A4 => Some(Action::Media(MediaControl::PlayPause)),
      B2 => Some(Action::Media(MediaControl::VolumeUp)),
      B3 => Some(Action::Media(MediaControl::NextTrack)),
      B4 => Some(Action::Media(MediaControl::Eject))
    );
  }

  #[test]
  fn meta_1_with_regular() {
    test_transitions!(State::default();
      M1 => None::<Action>,
      A2 => Some(Action::Short(0)),
      M1 => None::<Action>,
      A3 => Some(Action::Short(1)),
      M1 => None::<Action>,
      A4 => Some(Action::Short(2)),
      M1 => None::<Action>,
      B2 => Some(Action::Short(3)),
      M1 => None::<Action>,
      B3 => Some(Action::Short(4)),
      M1 => None::<Action>,
      B4 => Some(Action::Short(5))
    );
  }

  #[test]
  fn meta_2_with_regular() {
    test_transitions!(State::default();
      M2 => None::<Action>,
      A2 => Some(Action::Short(6)),
      M2 => None::<Action>,
      A3 => Some(Action::Short(7)),
      M2 => None::<Action>,
      A4 => Some(Action::Short(8)),
      M2 => None::<Action>,
      B2 => Some(Action::Short(9)),
      M2 => None::<Action>,
      B3 => Some(Action::Short(10)),
      M2 => None::<Action>,
      B4 => Some(Action::Short(11))
    );
  }

  #[test]
  fn meta_1_with_meta_2() {
    test_transitions!(State::default();
      M1 => None::<Action>,
      M2 => None::<Action>,
      A2 => Some(Action::Short(0)),
      M1 => None::<Action>,
      M2 => None::<Action>,
      A3 => Some(Action::Short(1)),
      M1 => None::<Action>,
      M2 => None::<Action>,
      A4 => Some(Action::Short(2)),
      M1 => None::<Action>,
      M2 => None::<Action>,
      B2 => Some(Action::Short(3)),
      M1 => None::<Action>,
      M2 => None::<Action>,
      B3 => Some(Action::Short(4)),
      M1 => None::<Action>,
      M2 => None::<Action>,
      B4 => Some(Action::Short(5))
    );
  }

  #[test]
  fn meta_2_with_meta_1() {
    test_transitions!(State::default();
      M2 => None::<Action>,
      M1 => None::<Action>,
      A2 => Some(Action::Short(6)),
      M2 => None::<Action>,
      M1 => None::<Action>,
      A3 => Some(Action::Short(7)),
      M2 => None::<Action>,
      M1 => None::<Action>,
      A4 => Some(Action::Short(8)),
      M2 => None::<Action>,
      M1 => None::<Action>,
      B2 => Some(Action::Short(9)),
      M2 => None::<Action>,
      M1 => None::<Action>,
      B3 => Some(Action::Short(10)),
      M2 => None::<Action>,
      M1 => None::<Action>,
      B4 => Some(Action::Short(11))
    );
  }

  #[test]
  fn meta_1_with_meta_1() {
    test_transitions!(State::default();
      M1 => None::<Action>,
      M1 => None::<Action>,
      A2 => Some(Action::Short(0)),
      M1 => None::<Action>,
      M1 => None::<Action>,
      A3 => Some(Action::Short(1)),
      M1 => None::<Action>,
      M1 => None::<Action>,
      A4 => Some(Action::Short(2)),
      M1 => None::<Action>,
      M1 => None::<Action>,
      B2 => Some(Action::Short(3)),
      M1 => None::<Action>,
      M1 => None::<Action>,
      B3 => Some(Action::Short(4)),
      M1 => None::<Action>,
      M1 => None::<Action>,
      B4 => Some(Action::Short(5))
    );
  }

  #[test]
  fn meta_2_with_meta_2() {
    test_transitions!(State::default();
      M2 => None::<Action>,
      M2 => None::<Action>,
      A2 => Some(Action::Short(6)),
      M2 => None::<Action>,
      M2 => None::<Action>,
      A3 => Some(Action::Short(7)),
      M2 => None::<Action>,
      M2 => None::<Action>,
      A4 => Some(Action::Short(8)),
      M2 => None::<Action>,
      M2 => None::<Action>,
      B2 => Some(Action::Short(9)),
      M2 => None::<Action>,
      M2 => None::<Action>,
      B3 => Some(Action::Short(10)),
      M2 => None::<Action>,
      M2 => None::<Action>,
      B4 => Some(Action::Short(11))
    );
  }

  #[test]
  fn meta_1_with_meta_1_with_regular() {
    let mut state = State::default();

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A2), Some(Action::Short(0)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A3), Some(Action::Short(1)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A4), Some(Action::Short(2)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B2), Some(Action::Short(3)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B3), Some(Action::Short(4)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B4), Some(Action::Short(5)));
  }

  #[test]
  fn meta_2_with_meta_2_with_regular() {
    let mut state = State::default();

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A2), Some(Action::Short(6)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A3), Some(Action::Short(7)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A4), Some(Action::Short(8)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(A2), Some(Action::Short(6)));
    assert_eq!(state.transition(A3), Some(Action::Media(MediaControl::PrevTrack)));
    assert_eq!(state.transition(A4), Some(Action::Media(MediaControl::PlayPause)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B2), Some(Action::Short(9)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B3), Some(Action::Short(10)));

    assert_eq!(state.transition(M2), None);
    assert_eq!(state.transition(B4), Some(Action::Short(11)));
  }

  #[test]
  fn meta_1_with_regular_with_meta_1() {
    let mut state = State::default();

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A2), Some(Action::Short(0)));
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A3), Some(Action::Short(1)));
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A4), Some(Action::Short(2)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A2), Some(Action::Short(0)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A3), Some(Action::Short(1)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(A4), Some(Action::Short(2)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B2), Some(Action::Short(3)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B3), Some(Action::Short(4)));

    assert_eq!(state.transition(M1), None);
    assert_eq!(state.transition(B4), Some(Action::Short(5)));
  }
}
