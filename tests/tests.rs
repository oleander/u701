#![feature(assert_matches)]

use std::assert_matches::assert_matches;
use u701::*;

#[test]
// [Pressed] Up -> Down
fn test_up_to_down() {
  let state = PushState::Up(0);
  let event: ClickEvent = [0, 0, BUTTON_2, 0];
  let (next_state, next_event) = state.transition(&event);

  assert_matches!(next_state, PushState::Down(BUTTON_2));
  assert_matches!(next_event, Some(_));
}

#[test]
// [Released] Down -> Up
fn test_down_to_up() {
  let state = PushState::Down(BUTTON_2);
  let event: ClickEvent = [0, 0, 0, 0];
  let (next_state, next_event) = state.transition(&event);

  assert_matches!(next_state, PushState::Up(BUTTON_2));
  assert_matches!(next_event, None);
}

// focus on only this test
#[test]
// [Invalid] Down -> Down
fn test_down_to_down() {
  let state = PushState::Down(BUTTON_2);
  let event: ClickEvent = [0, 0, BUTTON_3, 0];
  let (next_state, next_event) = state.transition(&event);

  assert_matches!(next_state, PushState::Down(BUTTON_2));
  assert_matches!(next_event, None);
}

#[test]
// [Invalid] Up -> Up
fn test_up_to_up() {
  let state = PushState::Up(BUTTON_2);
  let event: ClickEvent = [0, 0, 0, 0];
  let (next_state, next_event) = state.transition(&event);

  assert_matches!(next_state, PushState::Up(BUTTON_2));
  assert_matches!(next_event, None);
}

#[test]
// [Pressed] Up(Meta) -> Down
fn test_meta_up_to_regular_down() {
  let prev_state = PushState::Up(META_1);
  let curr_event: ClickEvent = [0, 0, BUTTON_2, 0];
  let (next_state, next_event) = prev_state.transition(&curr_event);

  assert_matches!(next_state, PushState::Down(BUTTON_2));
  assert_matches!(next_event, Some(_));
}

#[test]
// [Invalid] Up(Meta) -> Up
fn test_meta_up_to_regular_up() {
  let prev_state = PushState::Up(META_1);
  let curr_event: ClickEvent = [0, 0, 0, 0];
  let (next_state, next_event) = prev_state.transition(&curr_event);

  assert_matches!(next_state, PushState::Up(META_1));
  assert_matches!(next_event, None);
}
