
#include <Arduino.h>
#include "LissajousApp.h"
#include <math.h>
#include "App.h"
#include "Menu.h"
#include "Buttons.h"

namespace {
static uint8_t wrapIndex(int16_t value, uint8_t len) {
  while (value < 0) {
    value += len;
  }
  while (value >= len) {
    value -= len;
  }
  return (uint8_t)value;
}
}

LissajousApp::LissajousApp(const Menu* menu)
  : App(menu) {
}

void LissajousApp::onEnter(UIContext& ctx) {
  _editing = false;
  _selectionIndex = 0;
  _uiVisibleUntilMs = 0;
  applyConfiguredSampleRate(ctx);
}

uint32_t LissajousApp::computeSampleRate(uint16_t sampleSize, float windowMs) const {
  const uint32_t kMaxSampleRateHz = 200000;

  if (windowMs <= 0.0f) {
    return 1;
  }

  float requested = (sampleSize * 1000.0f) / windowMs;
  uint32_t rate = (uint32_t)requested;

  if (rate < 1) {
    rate = 1;
  }
  if (rate > kMaxSampleRateHz) {
    rate = kMaxSampleRateHz;
  }

  return rate;
}

void LissajousApp::applyConfiguredSampleRate(UIContext& ctx) const {
  uint16_t sampleSize = ctx.dsp->getSampleSize();
  uint32_t sampleRate = computeSampleRate(sampleSize, _time.values[_timeIndex]);
  ctx.dsp->changeSampleRate(sampleRate);
}

void LissajousApp::markUIActivity() {
  _uiVisibleUntilMs = millis() + kUIVisibleMs;
}

bool LissajousApp::shouldShowUI() const {
  return (int32_t)(millis() - _uiVisibleUntilMs) < 0;
}

void LissajousApp::drawOverlayUI(UIContext& ctx) {
  auto d = ctx.display;

  d->fillRect(0, 0, 128, 10, SH110X_BLACK);
  d->setTextSize(1);
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  d->setCursor(0, 0);

  if (!_editing && _selectionIndex == 0) {
    d->setTextColor(SH110X_BLACK, SH110X_WHITE);
  }
  d->print("t:");
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  if (_selectionIndex == 0) {
    d->setTextColor(SH110X_BLACK, SH110X_WHITE);
  }
  d->print(_time.names[_timeIndex]);

  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  d->setCursor(62, 0);
  if (!_editing && _selectionIndex == 1) {
    d->setTextColor(SH110X_BLACK, SH110X_WHITE);
  }
  d->print("A:");
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  if (_selectionIndex == 1) {
    d->setTextColor(SH110X_BLACK, SH110X_WHITE);
  }
  d->print(_amplitude.names[_amplitudeIndex]);
  d->print("V");

  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
}

void LissajousApp::processInput(UIContext& ctx) {
  bool encShort = ctx.buttons->shortPressed(ENC_BTN);
  bool backShort = ctx.buttons->shortPressed(BACK_BTN);
  int8_t delta = ctx.encoderDelta;

  if (delta != 0 || encShort || backShort) {
    markUIActivity();
  }

  if (!_editing) {
    if (delta != 0) {
      _selectionIndex = wrapIndex((int16_t)_selectionIndex + delta, LISSA_APP_MAX_UI_ITEMS);
    }
    if (encShort) {
      _editing = true;
    }
    return;
  }

  if (delta != 0) {
    switch (_selectionIndex) {
      case 0:
        _timeIndex = wrapIndex((int16_t)_timeIndex + delta, _time.len);
        applyConfiguredSampleRate(ctx);
        break;
      case 1:
        _amplitudeIndex = wrapIndex((int16_t)_amplitudeIndex + delta, _amplitude.len);
        break;
    }
  }

  if (encShort || backShort) {
    _editing = false;
  }
}

void LissajousApp::draw(UIContext& ctx, bool fullRedraw) {
  (void)fullRedraw;

  processInput(ctx);

  const uint8_t CENTER_Y = 32;
  const uint8_t CENTER_X = 63;
  const int16_t HALF_W = 63;
  const int16_t HALF_H = 31;
  const float voltsPerRange = _amplitude.values[_amplitudeIndex];

  auto d = ctx.display;
  auto dsp = ctx.dsp;
  d->clearDisplay();

  float* buffer0 = dsp->getChannel(0)->getWaveformVolts();
  float* buffer1 = dsp->getChannel(1)->getWaveformVolts();
  uint16_t sampleSize = dsp->getSampleSize();

  for (uint16_t i = 0; i < sampleSize; i++) {
    int16_t x = (int16_t)lroundf(CENTER_X + (buffer0[i] / voltsPerRange) * HALF_W);
    int16_t y = (int16_t)lroundf(CENTER_Y - (buffer1[i] / voltsPerRange) * HALF_H);

    if (x >= 0 && x < 128 && y >= 0 && y < 64) {
      d->drawPixel(x, y, SH110X_WHITE);
    }
  }

  if (shouldShowUI()) {
    drawOverlayUI(ctx);
  }

  d->display();
}
