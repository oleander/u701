#![no_std]

// use core::panic::PanicInfo;
pub type ID = u8;

struct MediaKeyReport(u8, u8);

const KEY_MEDIA_NEXT_TRACK: MediaKeyReport = MediaKeyReport(1, 0);
const KEY_MEDIA_PREVIOUS_TRACK: MediaKeyReport = MediaKeyReport(2, 0);
const KEY_MEDIA_STOP: MediaKeyReport = MediaKeyReport(4, 0);
const KEY_MEDIA_PLAY_PAUSE: MediaKeyReport = MediaKeyReport(8, 0);
const KEY_MEDIA_EJECT: MediaKeyReport = MediaKeyReport(16, 0);
const KEY_MEDIA_VOLUME_UP: MediaKeyReport = MediaKeyReport(32, 0);
const KEY_MEDIA_VOLUME_DOWN: MediaKeyReport = MediaKeyReport(64, 0);
const KEY_MEDIA_WWW_HOME: MediaKeyReport = MediaKeyReport(128, 0);
const KEY_MEDIA_LOCAL_MACHINE_BROWSER: MediaKeyReport = MediaKeyReport(0, 1);
const KEY_MEDIA_CALCULATOR: MediaKeyReport = MediaKeyReport(0, 2);
const KEY_MEDIA_WWW_BOOKMARKS: MediaKeyReport = MediaKeyReport(0, 4);
const KEY_MEDIA_WWW_SEARCH: MediaKeyReport = MediaKeyReport(0, 8);
const KEY_MEDIA_WWW_STOP: MediaKeyReport = MediaKeyReport(0, 16);
const KEY_MEDIA_WWW_BACK: MediaKeyReport = MediaKeyReport(0, 32);
const KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION: MediaKeyReport = MediaKeyReport(0, 64);
const KEY_MEDIA_EMAIL_READER: MediaKeyReport = MediaKeyReport(0, 128);

const META_1: u8 = 8;
const META_2: u8 = 8;

enum BLEEvent {
  MediaKey(MediaKeyReport),
  Letter(u8),
}

const REGULAR_LOOKUP: HashMap<u8, [u8; 2]> = {
  let mut table = HashMap::new();
  table.insert(4, KEY_MEDIA_NEXT_TRACK);
  table.insert(50, KEY_MEDIA_NEXT_TRACK);
  table.insert(71, KEY_MEDIA_NEXT_TRACK);
  table
}

const META_LOOKUP_1: HashMap<u8, [u8; 2]> = {
  let mut table = HashMap::new();
  table.insert(4, KEY_MEDIA_NEXT_TRACK);
  table.insert(50, KEY_MEDIA_NEXT_TRACK);
  table.insert(71, KEY_MEDIA_NEXT_TRACK);
  table
}

const META_LOOKUP_2: HashMap<u8, [u8; 2]> = {
  let mut table = HashMap::new();
  table.insert(4, KEY_MEDIA_NEXT_TRACK);
  table.insert(50, KEY_MEDIA_NEXT_TRACK);
  table.insert(71, KEY_MEDIA_NEXT_TRACK);
  table
}

type ClickEvent = [u8; 4];

#[derive(Copy, Clone, Debug, PartialEq)]
pub enum State {
  Down(ID),
  Up(ID),
}

impl State {
  // Released: [0, 0, 0, 0]
  // Pressed:  [0, 0, ID, 0]
  pub fn update(&self, event: &ClickEvent) -> Self {
    match (event, *self) {
      // The button was released after being pressed
      // [Ok] Pressed -> Released (updated)
      ([.., 0, _], State::Down(id)) => State::Up(id),
      // A button has already been pushed
      // [User error] Pressed -> Pressed (ignored)
      ([.., _, _], State::Down(id)) => State::Down(id),

      // The button was released after being released
      // [Bug] Released -> Released (ignored)
      ([.., 0, _], State::Up(id)) => State::Up(id),

      // The button was pressed after being released
      // [Ok] Released -> Pressed (updated)
      ([.., id, _], State::Up(_)) => State::Down(*id),
    }
  }

  pub fn transition(&self, event: &ClickEvent) -> (Self, Option<BLEEvent>) {
    let next_state = self.update(event);
    let next_event = match (self, next_state) {
      (State::Up(META_1), State::Down(id)) => {
        META_LOOKUP_1.get(&id).map(|key| BLEEvent::MediaKey(*key))
      }

      (State::Up(META_2), State::Down(id)) => {
        META_LOOKUP_2.get(&id).map(|key| BLEEvent::MediaKey(*key))
      }

      (_, State::Down(id)) => {
        REGULAR_LOOKUP.get(&id).map(|key| BLEEvent::MediaKey(*key))
      },

      (_, State::Up(_)) => None
    }

    (next_state, next_event)
  }
}

impl Default for State {
  fn default() -> Self {
    State::Up(0)
  }
}
