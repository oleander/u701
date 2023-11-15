use crate::types::*;
use crate::constants::*;

impl From<MediaControlKey> for [u8; 2] {
  fn from(key: MediaControlKey) -> Self {
    [key.0, key.1]
  }
}

impl InputState {
  fn from(id: u8) -> Option<Self> {
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

  fn transition_to(&self, next: InputState) -> Result<(Option<BluetoothEvent>, InputState), InvalidButtonTransitionError> {
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
