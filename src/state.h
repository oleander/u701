#include "ota.h"
#include "shared.h"

enum class Action { TICK, RESTART, INIT_OTA, LOOP_OTA, WAIT_FOR_PHONE };

struct State {
  int pushedAt;
  ID id;
  bool active;
  Action action;

  int ranAgoInMillis() const;
  bool isActive() const;
  void setInactive();
  void setActive();
  void reset();
  void tick();
};
