#include "state.h"

bool State::isActive() const {
  return active;
};

void State::reset(){};

void State::tick() {
}

void State::setActive() {
  pushedAt = millis();
  active   = true;
}

void State::setInactive() {
  active = false;
}

int State::ranAgoInMillis() const {
  return millis() - pushedAt;
}
