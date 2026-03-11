#pragma once

#include <Arduino.h>
#define BUTTON_LONGPRESS_MS 300
#define BACK_BTN 1
#define ENC_BTN 0

class Buttons {
public:
  Buttons(const uint8_t* pins, uint8_t count, unsigned long debounceMs = 20);

  void begin();
  void update();

  bool state(uint8_t id);     // true = pressed
  bool pressed(uint8_t id);   // true only on press event
  bool released(uint8_t id);  // true only on release event
  bool anyPressed();
  bool anyReleased();
  bool shortPressed(uint8_t id);
  bool longPressed(uint8_t id);

private:
  uint8_t* _pins;
  uint8_t _count;

  unsigned long* _pressStartTime;
  bool* _shortPressEvent;
  bool* _longPressEvent;

  bool* _currentState;
  bool* _lastState;
  bool* _pressedEvent;
  bool* _releasedEvent;

  unsigned long* _lastDebounceTime;
  unsigned long _debounceDelay;
};
