#include "keyboard.h"

std::unordered_map<ID, OneButton *> buttons = {{BUTTON_A_A_RED, new OneButton(PIN, false)},
                                               {BUTTON_A_B_BLACK, new OneButton(PIN, false)},
                                               {BUTTON_A_C_BLUE, new OneButton(PIN, false)},
                                               {BUTTON_A_D_BLACK, new OneButton(PIN, false)},
                                               {BUTTON_B_A_RED, new OneButton(PIN, false)},
                                               {BUTTON_B_B_BLACK, new OneButton(PIN, false)},
                                               {BUTTON_B_C_BLUE, new OneButton(PIN, false)},
                                               {BUTTON_B_D_BLACK, new OneButton(PIN, false)}};

typedef OneButton *Button;
typedef u_int16_t ID;

enum FunctionState { NoFn, CmdFn, MediaFn };

FunctionState functionState = NoFn;

BleKeyboard keyboard(DEVICE_NAME, DEVICE_MANUFACTURER, DEVICE_BATTERY);

#define TEN_SECONDS 10000

void sendNOPKey(ID id) {
  Log.noticeln("[0x%x] NOP\n", id);
  keyboard.write(KEY_INVALID);
}

void sendKeyPress(char letter, ID id) {
  Log.noticeln("[%sFn] Sending letter %s", functionState == CmdFn ? "Cmd" : "Media", String(letter).c_str());

  keyboard.print(letter);
  functionState = NoFn;
}

void clickHandler(void *p) {
  ID id = POINTER(p);

  switch (id) {
  case BUTTON_A_A_RED:
    functionState = MediaFn;
    Log.noticeln("[0x%x] [Click] Media Fn\n", id);

    break;
  case BUTTON_A_B_BLACK:
    if (functionState == MediaFn) {
      sendKeyPress('2', id);
    } else if (functionState == CmdFn) {
      keyboard.write(KEY_ZOOM_IN);
      Log.noticeln("[0x%x] [Click] [CmdFn] Zoom In\n", id);
    } else {
      keyboard.write(KEY_MEDIA_VOLUME_DOWN);
      Log.noticeln("[0x%x] [Click] Volume Down\n", id);
    }

    break;
  case BUTTON_A_C_BLUE:
    if (functionState == MediaFn) {
      sendKeyPress('3', id);
    } else if (functionState == CmdFn) {
      keyboard.write(KEY_ZOOM_OUT);
      Log.noticeln("[0x%x] [Click] [CmdFn] Zoom Out\n", id);
    } else {
      keyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
      Log.noticeln("[0x%x] [Click] Previous Track\n", id);
    }

    break;
  case BUTTON_A_D_BLACK:
    keyboard.write(KEY_MEDIA_PLAY_PAUSE);
    Log.noticeln("[0x%x] [Click] Play/Pause\n", id);
    break;
  case BUTTON_B_A_RED:
    functionState = (functionState == CmdFn) ? NoFn : CmdFn;
    if (functionState == CmdFn) {
      Log.noticeln("[0x%x] [Click] CmdFn ON\n", id);
    } else {
      Log.noticeln("[0x%x] [Click] CmdFn OFF\n", id);
    }

    break;
  case BUTTON_B_B_BLACK:
    if (functionState == MediaFn) {
      sendKeyPress('6', id);
    } else if (functionState == CmdFn) {
      sendKeyPress('F', id);
    } else {
      keyboard.write(KEY_MEDIA_VOLUME_UP);
      Log.noticeln("[0x%x] [Click] Volume UP\n", id);
    }

    break;
  case BUTTON_B_C_BLUE:
    if (functionState == MediaFn) {
      sendKeyPress('7', id);
    } else if (functionState == CmdFn) {
      sendKeyPress('G', id);
    } else {
      keyboard.write(KEY_MEDIA_NEXT_TRACK);
      Log.noticeln("[0x%x] [Click] Next Track\n", id);
    }

    break;
  case BUTTON_B_D_BLACK:
    if (functionState == MediaFn) {
      sendKeyPress('8', id);
    } else if (functionState == CmdFn) {
      sendKeyPress('H', id);
    } else {
      keyboard.print('N');
    }

    break;
  default:
    sendNOPKey(id);
  }
}

void doubleClickHandler(void *p) {
  if (!keyboard.isConnected())
    return;

  ID id = POINTER(p);

  switch (id) {
  case BUTTON_B_D_BLACK:
    keyboard.write(KEY_MEDIA_EJECT);
    Log.noticeln("[0x%x] [Double] Eject\n", id);

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
