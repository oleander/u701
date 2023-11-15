#include <OneButton.h>

const int numberOfButtons       = 8;
const int pins[numberOfButtons] = {2, 3, 4, 5, 6, 7, 8, 9};
OneButton buttons[numberOfButtons];

extern "C" void handle_button_click(int buttonIndex);

static void handleClick(void *param) {
  handle_button_click(static_cast<int>(reinterpret_cast<intptr_t>(param)));
}

void setup() {
  for (int i = 0; i < numberOfButtons; ++i) {
    buttons[i] = OneButton(pins[i]);
  }

  for (int i = 0; i < numberOfButtons; ++i) {
    buttons[i].attachClick(handleClick, reinterpret_cast<void *>(static_cast<intptr_t>(i)));
  }
}

void loop() {
  for (int i = 0; i < numberOfButtons; ++i) {
    buttons[i].tick();
  }
}