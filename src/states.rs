mod states {
  use crate::{BLEEvent, ClickEvent, *};
  use crate::BUTTON_2;

  // Define individual states as structs.
  pub struct Up(pub u8);
  pub struct Down(pub u8);

  impl Up {
    pub fn transition(self, event: &ClickEvent) -> Transition {
      use Transition::*;

      match event {
        [.., 0, _] => Stay(Box::new(self)), // No state change on Up -> Up
        [.., id, _] => MoveToDown(Down(*id))
      }
    }
  }

  impl Down {
    pub fn transition(self, event: &ClickEvent) -> Transition {
      use Transition::*;

      match event {
        [.., 0, _] => MoveToUp(Up(self.0)), // State change on Down -> Up
        [.., _, _] => Stay(Box::new(self))  // No state change on Down -> Down
      }
    }
  }

  // Define possible transitions including staying in the same state.
  pub enum Transition {
    Stay(Box<dyn CurrentState>),
    MoveToUp(Up),
    MoveToDown(Down)
  }

  pub trait CurrentState {
    fn to_event(&self) -> Option<BLEEvent>;
  }

  impl CurrentState for Up {
    fn to_event(&self) -> Option<BLEEvent> {
      // Logic to determine event for Up state
      None
    }
  }

  impl CurrentState for Down {
    fn to_event(&self) -> Option<BLEEvent> {
      // Logic to determine event for Down state
      Some(BLEEvent::Letter(self.0))
    }
  }
}

use states::*;

fn process_event(state: Box<dyn CurrentState>, event: &ClickEvent) -> Box<dyn CurrentState> {
  match state.transition(event) {
    Transition::Stay(s) => s,
    Transition::MoveToUp(s) => Box::new(s),
    Transition::MoveToDown(s) => Box::new(s)
  }
}

use crate::BUTTON_2;
use crate::ClickEvent;
use std::assert_matches::assert_matches;

#[test]
fn test_up_to_down() {
  let state = Box::new(Up(0));
  let event: ClickEvent = [0, 0, BUTTON_2, 0];
  let next_state = process_event(state, &event);

  assert_matches!(next_state, Box::new(Down(BUTTON_2)));
}
