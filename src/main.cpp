#include <Arduino.h>
#include <ArduinoLog.h>
#include <OneButton.h>

const int numberOfButtons       = 1;
const int pins[numberOfButtons] = {10};//, 8, 10, 14, 15, 16, 19};
OneButton buttons[numberOfButtons];

extern "C" void rust_handle_button_click(int buttonIndex);
extern "C" void init(void);

static void handleClick(void *param) {
  rust_handle_button_click(static_cast<int>(reinterpret_cast<intptr_t>(param)));
}

// int main(void) {
//   setup();

//   Log.notice("[main] Entering main loop");

//   while (true) {
//     loop();
//   }
// }

void app_main(void) {
  setup();

  Log.notice("[main] Entering main loop");

  while (true) {
    loop();
  }
}

void setup() {
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);

  Log.notice("[setup] Starting up main.cpp");
  Log.notice("[setup] Initializing Rust");
  init(); // Rust
  Log.notice("[setup] Finished initializing Rust");

  for (int i = 0; i < numberOfButtons; ++i) {
    buttons[i] = OneButton(pins[i], true, true);
    buttons[i].attachClick(handleClick, reinterpret_cast<void *>(static_cast<intptr_t>(i)));
  }

  Log.notice("[setup] Finished setting up main.cpp");
}

void loop() {
  for (int i = 0; i < numberOfButtons; ++i) {
    buttons[i].tick();
  }

  delay(10);
}