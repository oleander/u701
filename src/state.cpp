#include "state.h"

bool State::isActive() const {
  return active;
};

void State::reset() {
  button->reset();
};

void State::tick() {
  button->tick(active);
}

void State::setActive() {
  pushedAt = millis();
  button->tick(true);
  active = true;
}

void State::setInactive() {
  button->tick(false);
  active = false;
}

int State::ranAgoInMillis() const {
  return millis() - pushedAt;
}
