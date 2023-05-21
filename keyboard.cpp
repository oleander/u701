#include "keyboard.h"

std::unordered_map<ID, OneButton *> buttons = {{BUTTON_A_A_BLACK, new OneButton(PIN, false, false)},
                                               {BUTTON_A_B_BLUE, new OneButton(PIN, false, false)},
                                               {BUTTON_A_C_BLACK, new OneButton(PIN, false, false)},
                                               {BUTTON_A_D_RED, new OneButton(PIN, false, false)},
                                               {BUTTON_B_A_BLACK, new OneButton(PIN, false, false)},
                                               {BUTTON_B_B_BLUE, new OneButton(PIN, false, false)},
                                               {BUTTON_B_C_BLACK, new OneButton(PIN, false, false)},
                                               {BUTTON_B_D_RED, new OneButton(PIN, false, false)}};

typedef OneButton *Button;
typedef u_int16_t ID;

bool useKeyboardForLogging = false;
bool fn                    = false;
bool mediaFn               = false;

BleKeyboard keyboard(DEVICE_NAME, "701", 100);

void sendNOPKey(ID id) {
  PRINTF("[0x%x] NOP\n", id);
  keyboard.write(KEY_INVALID);
}

void sendFnKeyPress(char letter) {
  keyboard.press(KEY_LEFT_SHIFT);
  keyboard.press(KEY_LEFT_CTRL);
  keyboard.print(letter);
  delay(50);
  keyboard.releaseAll();
}

void sendMediaFnKeyPress(char letter, ID id) {
  if (!mediaFn) {
    PRINTLN("0x%x] [BUG] MediaFn is not enabled using letter %s", id, letter);
    return;
  }

  keyboard.press(KEY_LEFT_CTRL);
  keyboard.print(letter);
  delay(50);
  keyboard.releaseAll();
  PRINTF("[0x%x] [Click] [MediaFn] %s\n", id, letter);
  mediaFn = false;
}

void clickHandler(void *p) {
  ID id = POINTER(p);

  switch (id) {
  case BUTTON_A_A_BLACK:
    if (fn) {
      keyboard.print('C');
      PRINTF("[0x%x] [Click] [Fn] Restore map\n", id);
    } else {
      mediaFn = !mediaFn;
      PRINTF("[0x%x] [Click] Media Fn\n", id);
    }
    break;
  case BUTTON_A_B_BLUE:
    if (mediaFn) {
      sendMediaFnKeyPress('1', id);
    } else if (fn) {
      keyboard.write(KEY_ZOOM_IN);
      PRINTF("[0x%x] [Click] [Fn] Zoom In\n", id);
    } else {
      keyboard.write(KEY_MEDIA_VOLUME_DOWN);
      PRINTF("[0x%x] [Click] Volume Down\n", id);
    }
    break;
  case BUTTON_A_C_BLACK:
    if (mediaFn) {
      sendMediaFnKeyPress('2', id);
    } else if (fn) {
      keyboard.write(KEY_ZOOM_OUT);
      PRINTF("[0x%x] [Click] [Fn] Zoom Out\n", id);
    } else {
      keyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
      PRINTF("[0x%x] [Click] Previous Track\n", id);
    }
    break;
  case BUTTON_A_D_RED:
    keyboard.write(KEY_MEDIA_PLAY_PAUSE);
    PRINTF("[0x%x] [Click] Play/Pause\n", id);
    break;
  case BUTTON_B_A_BLACK:
    fn = !fn;

    if (fn) {
      // sendFnKeyPress('1');
      PRINTF("[0x%x] [Click] Fn ON\n", id);
    } else {
      // sendFnKeyPress('0');
      PRINTF("[0x%x] [Click] Fn OFF\n", id);
    }

    break;
  case BUTTON_B_B_BLUE:
    if (mediaFn) {
      sendMediaFnKeyPress('3', id);
    } else if (fn) {
      sendFnKeyPress('A');
      PRINTF("[0x%x] [Click] [Fn] Random Music\n", id);
    } else {
      keyboard.write(KEY_MEDIA_VOLUME_UP);
      PRINTF("[0x%x] [Click] Volume UP\n", id);
    }
    break;
  case BUTTON_B_C_BLACK:
    if (mediaFn) {
      sendMediaFnKeyPress('4', id);
    } else if (fn) {
      sendFnKeyPress('B');
      PRINTF("[0x%x] [Click] [Fn] Random Podcast\n", id);
    } else {
      keyboard.write(KEY_MEDIA_NEXT_TRACK);
      PRINTF("[0x%x] [Click] Next Track\n", id);
    }
    break;
  case BUTTON_B_D_RED:
    if (mediaFn) {
      sendMediaFnKeyPress('5', id);
    } else if (fn) {
      sendFnKeyPress('E');
      PRINTF("[0x%x] [Click] [Fn] Navigation\n", id);
    } else {
      sendFnKeyPress('N');
      PRINTF("[0x%x] [Click] Toggle noise cancelling\n", id);
    }
    break;
  default:
    sendNOPKey(id);
  }
}

void doubleClickHandler(void *p) {
  if (!keyboard.isConnected()) return;

  ID id = POINTER(p);

  switch (id) {
  case BUTTON_A_A_BLACK:
    sendFnKeyPress('D');
    PRINTF("[0x%x] [Double] Podcast\n", id);
    break;
  case BUTTON_A_B_BLUE:
    sendNOPKey(id);
    break;
  case BUTTON_A_C_BLACK:
    sendNOPKey(id);
    break;
  case BUTTON_A_D_RED:
    sendNOPKey(id);
    break;
  case BUTTON_B_A_BLACK:
    sendNOPKey(id);
    break;
  case BUTTON_B_B_BLUE:
    sendNOPKey(id);
    break;
  case BUTTON_B_C_BLACK:
    sendNOPKey(id);
    break;
  case BUTTON_B_D_RED:
    keyboard.write(KEY_MEDIA_EJECT);
    PRINTF("[0x%x] [Double] Eject\n", id);
    break;
  default:
    sendNOPKey(id);
  }
}

void setupButtons() {
  for (auto &[id, btn]: buttons) {
    auto point = reinterpret_cast<void *>(static_cast<uintptr_t>(id));

    btn->attachClick(clickHandler, point);

    // Enable double click for some of the buttons
    if (id == BUTTON_A_A_BLACK || id == BUTTON_B_D_RED) {
      btn->attachDoubleClick(doubleClickHandler, point);
      btn->setClickTicks(CLICK_TICKS);
    }

    btn->setDebounceTicks(DEBOUNCE_TICKS);
  }
}
