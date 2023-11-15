#[derive(Clone, Debug, Copy)]
pub struct MediaControlKey(pub u8, pub u8);

#[derive(Debug)]
pub enum InvalidButtonTransitionError {
  InvalidButton(InputState, InputState)
}

#[derive(Eq, Hash, PartialEq, Clone, Debug, Copy)]
pub enum ButtonIdentifier {
  A2,
  A3,
  A4,
  B2,
  B3,
  B4
}

#[derive(Clone, Debug, Copy)]
pub enum MetaButton {
  M1,
  M2
}

#[derive(Clone, Debug, Copy)]
pub enum InputState {
  Meta(MetaButton),
  Regular(ButtonIdentifier),
  Undefined
}

#[derive(Clone, Debug, Copy)]
pub enum BluetoothEvent {
  MediaControlKey(MediaControlKey),
  Letter(u8)
}
