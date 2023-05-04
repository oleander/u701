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

BleKeyboard keyboard(DEVICE_NAME, "701", 100);

void sendNOPKey(ID id) {
  PRINTF("[0x%x] NOP\n", id);
  keyboard.write(KEY_INVALID);
}

void sendFnKeyPress(char letter) {
  keyboard.press(KEY_LEFT_SHIFT);
  keyboard.press(KEY_LEFT_CTRL);
  keyboard.print(letter);
  delay(100);
  keyboard.releaseAll();
}

void handleDisconnectedClicks(ID id) {
  switch (id) {
  case BUTTON_A_D_RED:
    PRINTLN("Will restart ESP, hold on ...");
    ESP.restart();
    break;
  case BUTTON_B_D_RED:
    PRINTLN("Enable logging over bluetooth");
    useKeyboardForLogging = !useKeyboardForLogging;
    break;
  case BUTTON_B_B_BLUE:
    PRINTLN("Enable OTA ...");
    state = SETUP_OTA;
    break;
  default:
    sendNOPKey(id);
  }
}

void clickHandler(void *p) {
  ID id = POINTER(p);

  if (!keyboard.isConnected()) {
    handleDisconnectedClicks(id);
    return;
  }

  switch (id) {
  case BUTTON_A_A_BLACK:
    keyboard.write(KEY_MEDIA_PLAY_PAUSE);
    PRINTF("[0x%x] [Click] Play/Pause\n", id);
    break;
  case BUTTON_A_B_BLUE:
    keyboard.write(KEY_MEDIA_NEXT_TRACK);
    PRINTF("[0x%x] [Click] Next Track\n", id);
    break;
  case BUTTON_A_C_BLACK:
    keyboard.write(KEY_MEDIA_VOLUME_UP);
    PRINTF("[0x%x] [Click] Volume Up\n", id);
    break;
  case BUTTON_A_D_RED:
    keyboard.write(KEY_MEDIA_EJECT);
    PRINTF("[0x%x] [Click] Toggle keyboard\n", id);
    break;
  case BUTTON_B_A_BLACK:
    sendFnKeyPress('H');
    PRINTF("[0x%x] [Click] Help\n", id);
    break;
  case BUTTON_B_B_BLUE:
    keyboard.write(KEY_ZOOM_OUT);
    PRINTF("[0x%x] [Click] Zoom out\n", id);
    break;
  case BUTTON_B_C_BLACK:
    sendFnKeyPress('R');
    PRINTF("[0x%x] [Click] Play music\n", id);
    break;
  case BUTTON_B_D_RED:
    sendFnKeyPress('N');
    PRINTF("[0x%x] [Click] Toggle noise cancelling\n", id);
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
    sendNOPKey(id);
    break;
  case BUTTON_A_B_BLUE:
    keyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
    PRINTF("[0x%x] [Double] Previous track\n", id);
    break;
  case BUTTON_A_C_BLACK:
    keyboard.write(KEY_MEDIA_VOLUME_DOWN);
    PRINTF("[0x%x] [Double] Volume Down\n", id);
    break;
  case BUTTON_A_D_RED:
    sendNOPKey(id);
    break;
  case BUTTON_B_A_BLACK:
    sendNOPKey(id);
    break;
  case BUTTON_B_B_BLUE:
    keyboard.write(KEY_ZOOM_IN);
    PRINTF("[0x%x] [Double] Zoom in\n", id);
    break;
  case BUTTON_B_C_BLACK:
    sendFnKeyPress('P');
    PRINTF("[0x%x] [Double] Play podcast\n", id);
    break;
  case BUTTON_B_D_RED:
    sendNOPKey(id);
    break;
  default:
    sendNOPKey(id);
  }
}

void setupButtons() {
  for (auto &[id, btn]: buttons) {
    auto point = reinterpret_cast<void *>(static_cast<uintptr_t>(id));

    btn->attachClick(clickHandler, point);
    btn->attachDoubleClick(doubleClickHandler, point);
    btn->setDebounceTicks(DEBOUNCE_TICKS);
    btn->setClickTicks(CLICK_TICKS);
  }
}
