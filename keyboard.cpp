#include "keyboard.h"

std::unordered_map<ID, OneButton *> buttons = {{BUTTON_A_D_BLACK, new OneButton(PIN, false, false)},
                                               {BUTTON_A_C_BLUE, new OneButton(PIN, false, false)},
                                               {BUTTON_A_B_BLACK, new OneButton(PIN, false, false)},
                                               {BUTTON_A_A_RED, new OneButton(PIN, false, false)},
                                               {BUTTON_B_D_BLACK, new OneButton(PIN, false, false)},
                                               {BUTTON_B_C_BLUE, new OneButton(PIN, false, false)},
                                               {BUTTON_B_B_BLACK, new OneButton(PIN, false, false)},
                                               {BUTTON_B_A_RED, new OneButton(PIN, false, false)}};

typedef OneButton *Button;
typedef u_int16_t ID;

bool useKeyboardForLogging = false;

enum FunctionState { NoFn, CmdFn, MediaFn };

FunctionState functionState = NoFn;

BleKeyboard keyboard(DEVICE_NAME, "701", 100);

void sendNOPKey(ID id) {
  PRINTF("[0x%x] NOP\n", id);
  keyboard.write(KEY_INVALID);
}

void sendCmdFnKeyPress(char letter) { keyboard.print(letter); }

void sendMediaFnKeyPress(char letter, ID id) {
  if (functionState != MediaFn) {
    PRINTF("0x%x] [BUG] MediaFn is not enabled using letter %s\n", id, letter);
    return;
  }

  keyboard.print(letter);
  functionState = NoFn;
}

void clickHandler(void *p) {
  ID id = POINTER(p);

  switch (id) {
  case BUTTON_A_D_BLACK:
    keyboard.write(KEY_MEDIA_PLAY_PAUSE);
    PRINTF("[0x%x] [Click] Play/Pause\n", id);
    break;
  case BUTTON_A_C_BLUE:
    if (functionState == MediaFn) {
      sendMediaFnKeyPress('2', id);
    } else if (functionState == CmdFn) {
      keyboard.write(KEY_ZOOM_OUT);
      PRINTF("[0x%x] [Click] [CmdFn] Zoom Out\n", id);
    } else {
      keyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
      PRINTF("[0x%x] [Click] Previous Track\n", id);
    }

    break;
  case BUTTON_A_B_BLACK:
    if (functionState == MediaFn) {
      sendMediaFnKeyPress('1', id);
    } else if (functionState == CmdFn) {
      keyboard.write(KEY_ZOOM_IN);
      PRINTF("[0x%x] [Click] [CmdFn] Zoom In\n", id);
    } else {
      keyboard.write(KEY_MEDIA_VOLUME_DOWN);
      PRINTF("[0x%x] [Click] Volume Down\n", id);
    }

    break;
  case BUTTON_A_A_RED:
    if (functionState == CmdFn) {
      keyboard.print('C');
      PRINTF("[0x%x] [Click] [CmdFn] Restore map\n", id);
    } else {
      functionState = MediaFn;
      PRINTF("[0x%x] [Click] Media Fn\n", id);
    }
    break;
  case BUTTON_B_D_BLACK:
    if (functionState == MediaFn) {
      sendMediaFnKeyPress('5', id);
    } else if (functionState == CmdFn) {
      sendCmdFnKeyPress('E');
      PRINTF("[0x%x] [Click] [CmdFn] Navigation");
      PRINTF("[0x%x] [Click] [CmdFn] Navigation\n", id);
    } else {
      sendCmdFnKeyPress('N');
      PRINTF("[0x%x] [Click] Toggle noise cancelling\n", id);
    }
    break;
  case BUTTON_B_C_BLUE:
    if (functionState == MediaFn) {
      sendMediaFnKeyPress('4', id);
    } else if (functionState == CmdFn) {
      sendCmdFnKeyPress('B');
      PRINTF("[0x%x] [Click] [CmdFn] Random Podcast\n", id);
    } else {
      keyboard.write(KEY_MEDIA_NEXT_TRACK);
      PRINTF("[0x%x] [Click] Next Track\n", id);
    }
    break;
  case BUTTON_B_B_BLACK:
    if (functionState == MediaFn) {
      sendMediaFnKeyPress('3', id);
    } else if (functionState == CmdFn) {
      sendCmdFnKeyPress('A');
      PRINTF("[0x%x] [Click] [CmdFn] Random Music\n", id);
    } else {
      keyboard.write(KEY_MEDIA_VOLUME_UP);
      PRINTF("[0x%x] [Click] Volume UP\n", id);
    }
    break;
  case BUTTON_B_A_RED:
    functionState = (functionState == CmdFn) ? NoFn : CmdFn;

    if (functionState == CmdFn) {
      PRINTF("[0x%x] [Click] CmdFn ON\n", id);
    } else {
      PRINTF("[0x%x] [Click] CmdFn OFF\n", id);
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
  case BUTTON_B_D_BLACK:
    // keyboard.write(KEY_MEDIA_EJECT);
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

    // Enable double click & hold for some of the buttons
    if (id == BUTTON_B_D_BLACK) {
      btn->attachDoubleClick(doubleClickHandler, point);
      btn->setClickTicks(CLICK_TICKS);
    }

    btn->setDebounceTicks(DEBOUNCE_TICKS);
  }
}
