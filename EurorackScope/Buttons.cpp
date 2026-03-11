#include "Buttons.h"

#include <Arduino.h>
Buttons::Buttons(const uint8_t* pins, uint8_t count, unsigned long debounceMs) {
  _count = count;
  _debounceDelay = debounceMs;

  _pins = new uint8_t[_count];
  _currentState = new bool[_count];
  _lastState = new bool[_count];
  _pressedEvent = new bool[_count];
  _releasedEvent = new bool[_count];
  _lastDebounceTime = new unsigned long[_count];
  _pressStartTime = new unsigned long[_count];
  _shortPressEvent = new bool[_count];
  _longPressEvent = new bool[_count];
  for (uint8_t i = 0; i < _count; i++) {
    _pins[i] = pins[i];
    _currentState[i] = false;
    _lastState[i] = false;
    _pressedEvent[i] = false;
    _releasedEvent[i] = false;
    _lastDebounceTime[i] = 0;

    _pressStartTime[i] = 0;
    _shortPressEvent[i] = false;
    _longPressEvent[i] = false;
  }
}

void Buttons::begin() {
  for (uint8_t i = 0; i < _count; i++) {
    pinMode(_pins[i], INPUT_PULLUP);
  }
}

void Buttons::update() {
  for (uint8_t i = 0; i < _count; i++) {

    bool reading = (digitalRead(_pins[i]) == LOW);  // active LOW

    _pressedEvent[i] = false;
    _releasedEvent[i] = false;
    _shortPressEvent[i] = false;
    _longPressEvent[i] = false;


    if (reading != _lastState[i]) {
      _lastDebounceTime[i] = millis();
    }

    if ((millis() - _lastDebounceTime[i]) > _debounceDelay) {

      if (reading != _currentState[i]) {

        _currentState[i] = reading;

        if (_currentState[i]) {
          // Button pressed
          _pressedEvent[i] = true;
          _pressStartTime[i] = millis();
        } else {
          // Button released
          _releasedEvent[i] = true;

          unsigned long pressDuration = millis() - _pressStartTime[i];

          if (pressDuration >= BUTTON_LONGPRESS_MS) {
            _longPressEvent[i] = true;
          } else {
            _shortPressEvent[i] = true;
          }
        }
      }
    }

    _lastState[i] = reading;
  }
}

bool Buttons::state(uint8_t id) {
  if (id >= _count) return false;
  return _currentState[id];
}

bool Buttons::pressed(uint8_t id) {
  if (id >= _count) return false;
  return _pressedEvent[id];
}

bool Buttons::released(uint8_t id) {
  if (id >= _count) return false;
  return _releasedEvent[id];
}
bool Buttons::anyPressed() {
  for (uint8_t i = 0; i < _count; i++) {
    if (pressed(i)) {
      return true;
    }
  }
  return false;
}
bool Buttons::anyReleased() {
  for (uint8_t i = 0; i < _count; i++) {
    if (released(i)) {
      return true;
    }
  }
  return false;
}
bool Buttons::shortPressed(uint8_t id) {
  if (id >= _count) return false;
  return _shortPressEvent[id];
}

bool Buttons::longPressed(uint8_t id) {
  if (id >= _count) return false;
  return _longPressEvent[id];
}