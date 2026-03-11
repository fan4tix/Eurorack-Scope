
#include <Arduino.h>
#include "ScopeApp.h"
#include <math.h>
#include "App.h"
#include "Menu.h"
#include "Buttons.h"

int8_t scopeTrigger = 0;

ScopeApp::ScopeApp(const Menu* menu)
  : App(menu) {
}

void ScopeApp::draw(UIContext& ctx, bool fullRedraw) {
  processInput(ctx);
  auto d = ctx.display;
  auto dsp = ctx.dsp;
  const uint8_t CENTER = 37;
  float RANGE = (1.0 / _amplitude.values[_amplitudeIndex]);

  if (fullRedraw)
    d->clearDisplay();
  else
    d->fillRect(0, 9, 128, 63, SH110X_BLACK);
  drawUI(ctx);
  float* buffer = dsp->getWaveformFloat();

  float max = -9999;
  for (uint16_t i = 0; i < 128; i++) {
    //Serial.printf("x:%f",buffer[i]);
    d->drawPixel(i,
                 floor(CENTER - buffer[i * 2] * RANGE),
                 SH110X_WHITE);
    if (buffer[i * 2] > max) {
      max = buffer[i * 2];
    }
  }

  d->setCursor(0, 50);
  d->printf("Max: %.2fV", max);
  d->display();
}
void ScopeApp::drawUI(UIContext& ctx) {
  auto d = ctx.display;

  d->setTextSize(1);

  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  d->setCursor(0, 0);
  // Clear previous
  d->print("                                 ");
  d->setCursor(0, 0);
  if (!_editing && _selectionIndex == 0)
    d->setTextColor(SH110X_BLACK, SH110X_WHITE);
  d->print("CH:");
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);

  if (_selectionIndex == 0)
    d->setTextColor(SH110X_BLACK, SH110X_WHITE);
  d->print(_channel.names[_channelIndex]);
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  d->setCursor(50, 0);
  if (!_editing && _selectionIndex == 1)
    d->setTextColor(SH110X_BLACK, SH110X_WHITE);
  d->print("A:");
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  if (_selectionIndex == 1)
    d->setTextColor(SH110X_BLACK, SH110X_WHITE);
  d->print(_amplitude.names[_amplitudeIndex]);
  d->print("V");
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  d->setCursor(85, 0);
  if (!_editing && _selectionIndex == 2)
    d->setTextColor(SH110X_BLACK, SH110X_WHITE);
  d->print("t:");
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  if (_selectionIndex == 2)
    d->setTextColor(SH110X_BLACK, SH110X_WHITE);
  d->print(_time.names[_timeIndex]);
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
}
void ScopeApp::processInput(UIContext& ctx) {

  if (!_editing) {
    _selectionIndex += ctx.encoderDelta;
    _selectionIndex %= SCOPE_APP_MAX_UI_ITEMS;
    if (ctx.buttons->shortPressed(ENC_BTN)) {
      _editing = true;
    }
  } else {  // editing
    if (ctx.encoderDelta != 0) {
      switch (_selectionIndex) {
        case 0:  //Channel
          _channelIndex += ctx.encoderDelta;
          _channelIndex %= _channel.len;
          break;
        case 1:  // Amplitude
          _amplitudeIndex += ctx.encoderDelta;
          _amplitudeIndex %= _amplitude.len;
          break;
        case 2:  // Time
          _timeIndex += ctx.encoderDelta;
          _timeIndex %= _time.len;
          ctx.dsp->changeSampleRate(128 / _time.len);  // Reconfigure dsp for new time
          break;
      }
    }
    if (ctx.buttons->shortPressed(BACK_BTN) || ctx.buttons->shortPressed(ENC_BTN)) {
      _editing = false;
    }
  }
}
