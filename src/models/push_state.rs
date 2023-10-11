use crate::models::click_event::ClickEvent;
use crate::models::ble_event::BLEEvent;
use crate::models::id::ID;
use crate::con

#[derive(Copy, Clone, Debug, PartialEq)]
pub enum PushState {
  Down(ID),
  Up(ID),
}

impl PushState {
  // Allow us to keep track of the state of the button
  // @event The the click event (4 bytes)
  // @return 1. The new state of the button
  //         2. The event to send to the host
  //            1. MediaKeyReport (i.e Play/Pause)
  //            2. Letter (i.e. 'a')
  //            3. None (i.e. no event)
  pub fn transition(&self, event: &ClickEvent) -> (Self, Option<BLEEvent>) {
    use PushState::*;

    let next_state = match (event, *self) {
      // The button was released after being pressed
      // [Ok] Pressed -> Released (updated)
      ([.., 0, _], Down(id)) => Up(id),
      // A button has already been pushed
      // [User error] Pressed -> Pressed (ignored)
      ([.., _, _], Down(id)) => Down(id),

      // The button was released after being released
      // [Bug] Released -> Released (ignored)
      ([.., 0, _], Up(id)) => Up(id),

      // The button was pressed after being released
      // [Ok] Released -> Pressed (updated)
      ([.., id, _], Up(_)) => Down(*id),
    };

    let next_event = match (self, next_state) {
      // Meta key was pressed together with another key
      (Up(META_1), Down(id)) => META_LOOKUP_1.get(&id),

      // Meta key was pressed together with another key
      (Up(META_2), Down(id)) => META_LOOKUP_2.get(&id),

      // A regular key was pressed
      (_, Down(id)) => REGULAR_LOOKUP.get(&id),

      // A regular key was released
      (_, Up(_)) => None,
    };

    (next_state, next_event.cloned())
  }
}
