#include "keyboard.h"
#include <BleKeyboard.h>
#include <OneButton.h>
#include <unordered_map>

std::unordered_map<ID, OneButton *> buttons = {{BUTTON_A_A_BLACK, new OneButton(PIN, false, false)},
                                               {BUTTON_A_B_BLUE, new OneButton(PIN, false, false)},
                                               {BUTTON_A_C_BLACK, new OneButton(PIN, false, false)},
                                               {BUTTON_A_D_RED, new OneButton(PIN, false, false)},
                                               {BUTTON_B_A_BLACK, new OneButton(PIN, false, false)},
                                               {BUTTON_B_B_BLUE, new OneButton(PIN, false, false)},
                                               {BUTTON_B_C_BLACK, new OneButton(PIN, false, false)},
                                               {BUTTON_B_D_RED, new OneButton(PIN, false, false)}};

#define DEVICE_NAME "u701"

#define POINTER(p) (static_cast<uint16_t>(reinterpret_cast<uintptr_t>(p)))

typedef OneButton *Button;
typedef u_int16_t ID;

BleKeyboard keyboard(DEVICE_NAME, "701", 100);

void sendNOPKey(ID id)
{
  Serial1.print("[Keyboard] ");
  Serial1.printf("[0x%x] NOP\n", id);
  keyboard.write(KEY_INVALID);
}

void sendFnKeyPress(char letter)
{
  keyboard.write(KEY_ESC);
  keyboard.press(KEY_LEFT_SHIFT);
  keyboard.press(KEY_LEFT_CTRL);
  keyboard.print(letter);
  delay(100);
  keyboard.releaseAll();
}

void doubleClickHandler(void *p)
{
  if (!keyboard.isConnected())
    return;

  ID id = POINTER(p);

  switch (id)
  {
  case BUTTON_A_A_BLACK:
    sendNOPKey(id);
    break;
  case BUTTON_A_B_BLUE:
    keyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Double] Previous track\n", id);
    break;
  case BUTTON_A_C_BLACK:
    keyboard.write(KEY_MEDIA_VOLUME_DOWN);
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Double] Volume Down\n", id);
    break;
  case BUTTON_A_D_RED:
    sendFnKeyPress('M');
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Double] Cancel Siri\n", id);
    break;
  case BUTTON_B_A_BLACK:
    sendNOPKey(id);
    break;
  case BUTTON_B_B_BLUE:
    keyboard.write(KEY_ZOOM_IN);
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Double] Zoom in\n", id);
    break;
  case BUTTON_B_C_BLACK:
    sendFnKeyPress('P');
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Double] Play podcast\n", id);
    break;
  case BUTTON_B_D_RED:
    sendNOPKey(id);
    break;
  default:
    sendNOPKey(id);
  }
}

// Triple click (🅣)
void multiClickHandler(void *p)
{
  if (!keyboard.isConnected())
    return;

  ID id = POINTER(p);

  switch (id)
  {
  case BUTTON_A_A_BLACK:
    keyboard.write(KEY_ESC);
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Triple] ESC\n", id);
    break;
  case BUTTON_A_B_BLUE:
    keyboard.write(KEY_MEDIA_NEXT_TRACK);
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Triple] Next track\n", id);
    break;
  case BUTTON_A_C_BLACK:
    keyboard.write(KEY_MEDIA_VOLUME_UP);
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Triple] Volume Up\n", id);
    break;
  case BUTTON_A_D_RED:
    keyboard.write(KEY_MEDIA_EJECT);
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Triple] Eject\n", id);
    break;
  case BUTTON_B_A_BLACK:
    keyboard.write(KEY_HOME);
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Triple] Home\n", id);
    break;
  case BUTTON_B_B_BLUE:
    keyboard.write(KEY_ZOOM_OUT);
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Triple] Zoom out\n", id);
    break;
  case BUTTON_B_C_BLACK:
    sendFnKeyPress('S');
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Triple] Start Siri\n", id);
    break;
  case BUTTON_B_D_RED:
    keyboard.write(KEY_MEDIA_PLAY_PAUSE);
    Serial1.print("[Keyboard] ");
    Serial1.printf("[0x%x] [Triple] Play/Pause\n", id);
    break;
  default:
    sendNOPKey(id);
  }
}

void setupButtons()
{
  for (auto &[id, btn] : buttons)
  {
    auto point = reinterpret_cast<void *>(static_cast<uintptr_t>(id));

    btn->attachClick(doubleClickHandler, point);
    btn->attachDoubleClick(multiClickHandler, point);
    btn->attachLongPressStop(doubleClickHandler, point);
  }
}
