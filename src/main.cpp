#include <OneButton.h>

const int numberOfButtons       = 8;
const int pins[numberOfButtons] = {2, 3, 4, 5, 6, 7, 8, 9};
OneButton buttons[numberOfButtons];

static void handleClick(void *param) {
  OneButton *oneButton = static_cast<OneButton *>(param);
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
  // You will need to include code here that updates each button
  // For example:
  for (int i = 0; i < numberOfButtons; ++i) {
    buttons[i].tick();
  }
}