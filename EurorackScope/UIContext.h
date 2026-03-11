#pragma once

#include <Adafruit_SH110X.h>
#include "DSP.h"
#include "PSU.h"
#include "Buttons.h"
#include "PicoEncoder.h"

struct UIContext {
  Adafruit_SH1106G* display;
  DSP* dsp;
  PSU* psu;
  Buttons* buttons;
  int8_t encoderDelta;
};