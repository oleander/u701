#include "ota.h"
#include "shared.h"
#include <OneButton.h>

enum class Action { TICK, RESTART, INIT_OTA, LOOP_OTA, WAIT_FOR_PHONE };

struct State {
  int pushedAt;
  OTA ota;
  ID id;
  OneButton *button;
  bool active;
  Action action;

  int ranAgoInMillis() const;
  bool isActive() const;
  void setInactive();
  void setActive();
  void reset();
  void tick();
};
