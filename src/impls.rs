use crate::types::{BluetoothEvent, InputState, InvalidButtonTransitionError, MetaButton, *};
use crate::types::ButtonIdentifier::*;
use crate::constants::*;

impl From<MediaControlKey> for [u8; 2] {
  fn from(key: MediaControlKey) -> Self {
    [key.0, key.1]
  }
}

impl InputState {
  pub fn from(id: u8) -> Option<Self> {
    use InputState::*;
    use MetaButton::*;

    match id {
      1 => Some(Meta(M1)),
      2 => Some(Regular(A2)),
      3 => Some(Regular(A3)),
      4 => Some(Regular(A4)),
      5 => Some(Meta(M2)),
      6 => Some(Regular(B2)),
      7 => Some(Regular(B3)),
      8 => Some(Regular(B4)),
      _ => None
    }
  }

  pub fn transition_to(self, next: Self) -> Result<(Option<BluetoothEvent>, Self), InvalidButtonTransitionError> {
    use InvalidButtonTransitionError::*;
    use MetaButton::*;
    use InputState::*;

    let event = match (self, next) {
      // [INVALID] Meta -> Meta
      (from @ Meta(_), to @ Meta(_)) => return Err(InvalidButton(from.clone(), to)),

      // [OK] Meta 1 -> Regular
      (Meta(M1), Regular(button)) => META_BUTTON_EVENTS_ONE.get(&button),

      // [OK] Meta 2 -> Regular
      (Meta(M2), Regular(button)) => META_BUTTON_EVENTS_TWO.get(&button),

      // [OK] Regular -> Regular
      (_, Regular(button)) => REGULAR_BUTTON_EVENTS.get(&button),

      // [OK] Regular -> Meta
      (_, Meta(_)) => None,

      // [BUG] ?? -> Undefined
      (_, Undefined) => {
        panic!("[BUG] Cannot transition to undefined state")
      }
    };

    Ok((event.cloned(), next))
  }
}

#[cfg(test)]
mod tests {
  use super::*;

  #[test]
  fn test_input_state_from() {
    assert_eq!(InputState::from(0), Some(Meta(M1)));
    assert_eq!(InputState::from(1), Some(Regular(A2)));
    assert_eq!(InputState::from(2), Some(Regular(A3)));
    assert_eq!(InputState::from(3), Some(Regular(A4)));
    assert_eq!(InputState::from(4), Some(Meta(M2)));
    assert_eq!(InputState::from(5), Some(Regular(B2)));
    assert_eq!(InputState::from(6), Some(Regular(B3)));
    assert_eq!(InputState::from(7), Some(Regular(B4)));
    assert_eq!(InputState::from(8), None); 
  }

  #[test]
  fn test_input_state_transition_to() {
    let state = InputState::Regular(A2);
    let next_state = InputState::Regular(A3);
    let result = state.transition_to(next_state);
    assert_eq!(result, Ok((Some(BluetoothEvent::MediaNextTrack), next_state)));

    let state = InputState::Meta(M1);
    let next_state = InputState::Regular(A2);
    let result = state.transition_to(next_state);
    assert_eq!(result, Ok((Some(BluetoothEvent::MediaPlayPause), next_state)));

    let state = InputState::Regular(A2);
    let next_state = InputState::Meta(M1);
    let result = state.transition_to(next_state);
    assert_eq!(result, Err(InvalidButtonTransitionError::InvalidButton(state, next_state)));

    let state = InputState::Regular(A2);
    let next_state = InputState::Undefined;
    let result = state.transition_to(next_state);
    assert!(result.is_err());
  }

  #[test]
  fn test_media_control_key_into() {
    let key = MediaControlKey(1, 2);
    let result: [u8; 2] = key.into();
    assert_eq!(result, [1, 2]);
  }

  #[test]
  fn test_meta_button_events_one() {
    assert_eq!(META_BUTTON_EVENTS_ONE.get(&A2), Some(&BluetoothEvent::Letter(2)));
    assert_eq!(META_BUTTON_EVENTS_ONE.get(&A3), Some(&BluetoothEvent::Letter(3)));
    assert_eq!(META_BUTTON_EVENTS_ONE.get(&A4), Some(&BluetoothEvent::Letter(4)));
    assert_eq!(META_BUTTON_EVENTS_ONE.get(&B2), Some(&BluetoothEvent::Letter(6)));
    assert_eq!(META_BUTTON_EVENTS_ONE.get(&B3), Some(&BluetoothEvent::Letter(7)));
    assert_eq!(META_BUTTON_EVENTS_ONE.get(&B4), Some(&BluetoothEvent::Letter(8)));
  }

  #[test]
  fn test_meta_button_events_two() {
    assert_eq!(META_BUTTON_EVENTS_TWO.get(&A2), Some(&BluetoothEvent::Letter(10)));
    assert_eq!(META_BUTTON_EVENTS_TWO.get(&A3), Some(&BluetoothEvent::Letter(11)));
    assert_eq!(META_BUTTON_EVENTS_TWO.get(&A4), Some(&BluetoothEvent::Letter(12)));
    assert_eq!(META_BUTTON_EVENTS_TWO.get(&B2), Some(&BluetoothEvent::Letter(14)));
    assert_eq!(META_BUTTON_EVENTS_TWO.get(&B3), Some(&BluetoothEvent::Letter(15)));
    assert_eq!(META_BUTTON_EVENTS_TWO.get(&B4), Some(&BluetoothEvent::Letter(16)));
  }
}
