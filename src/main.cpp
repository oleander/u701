#include <OneButton.h>
#include <ArduinoLog.h>

const int numberOfButtons       = 8;
const int pins[numberOfButtons] = {2, 3, 4, 5, 6, 7, 8, 9};
OneButton buttons[numberOfButtons];

extern "C" void handle_button_click(int buttonIndex);
extern "C" void init(void);

static void handleClick(void *param) {
  handle_button_click(static_cast<int>(reinterpret_cast<intptr_t>(param)));
}

int main(void) {
  setup();

  Log.notice("[main] Entering main loop");

  while (true) {
    loop();
  }

  return 0;
}

void setup() {
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);

  Log.notice("[setup] Starting up main.cpp");
  Log.notice("[setup] Initializing Rust");
  init(); // Rust
  Log.notice("[setup] Finished initializing Rust");

  for (int i = 0; i < numberOfButtons; ++i) {
    buttons[i] = OneButton(pins[i]);
  }

  for (int i = 0; i < numberOfButtons; ++i) {
    buttons[i].attachClick(handleClick, reinterpret_cast<void *>(static_cast<intptr_t>(i)));
  }

  Log.notice("[setup] Finished setting up main.cpp");
}

void loop() {
  for (int i = 0; i < numberOfButtons; ++i) {
    buttons[i].tick();
  }
}