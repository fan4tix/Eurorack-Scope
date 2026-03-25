
#include <Arduino.h>
#include "ScopeApp.h"
#include <math.h>
#include "App.h"
#include "Menu.h"
#include "Buttons.h"

ScopeApp::ScopeApp(const Menu* menu)
  : App(menu) {
}

void ScopeApp::onEnter(UIContext& ctx) {
  _editing = false;
  _selectionIndex = 0;
  applyConfiguredSampleRate(ctx);
}

uint16_t ScopeApp::computeDisplaySamples(uint16_t sampleSize) const {
  const uint16_t kMinDisplaySamples = 128;
  if (sampleSize <= kMinDisplaySamples) {
    return sampleSize;
  }

  // Keep some headroom for trigger-based horizontal alignment.
  uint16_t displaySamples = (sampleSize * 3u) / 4u;
  if (displaySamples < kMinDisplaySamples) {
    displaySamples = kMinDisplaySamples;
  }
  if (displaySamples >= sampleSize) {
    displaySamples = sampleSize - 1;
  }
  return displaySamples;
}

uint16_t ScopeApp::findFirstRisingZeroCrossing(const float* waveformVolts, uint16_t searchEndExclusive) const {
  const float kZero = 0.0f;
  const float kHysteresisVolts = 0.02f;

  if (!waveformVolts || searchEndExclusive < 2) {
    return 0;
  }

  for (uint16_t i = 1; i < searchEndExclusive; ++i) {
    float prev = waveformVolts[i - 1];
    float curr = waveformVolts[i];
    if (prev <= (kZero - kHysteresisVolts) && curr >= (kZero + kHysteresisVolts)) {
      return i;
    }
  }

  return 0;
}

uint32_t ScopeApp::computeScopeSampleRate(uint16_t displaySamples, float windowMs) const {
  // Max per-channel rate with 2-ch round-robin: 48 MHz / (96 × 2) = 250 kHz.
  // Use 200 kHz for margin.
  const uint32_t kMaxSampleRateHz = 200000;
  if (windowMs <= 0.0f) {
    return 1;
  }

  // Full-screen capture window: sampleRate = displayedSampleCount / windowSeconds.
  float requested = (displaySamples * 1000.0f) / windowMs;
  uint32_t rate = (uint32_t)requested;

  if (rate < 1) {
    rate = 1;
  }
  if (rate > kMaxSampleRateHz) {
    rate = kMaxSampleRateHz;
  }

  return rate;
}

float ScopeApp::drawScopeTrace(Adafruit_SH1106G* display,
                               const float* waveformVolts,
                               uint16_t sampleSize,
                               uint16_t plotStart,
                               uint16_t displaySamples,
                               uint16_t plotWidth,
                               uint8_t plotTop,
                               uint8_t plotBottom,
                               uint8_t plotCenter,
                               float voltsToPixels) const {
  float vMin = 9999.0f;
  float vMax = -9999.0f;

  if (!waveformVolts || sampleSize == 0) {
    return 0.0f;
  }

  for (uint16_t i = 0; i < plotWidth; i++) {
    uint16_t sampleIndex = plotStart + ((uint32_t(i) * displaySamples) / plotWidth);
    if (sampleIndex >= sampleSize) {
      sampleIndex = sampleSize - 1;
    }

    float sampleVolts = waveformVolts[sampleIndex];
    int16_t y = floor(plotCenter - sampleVolts * voltsToPixels);
    if (y >= plotTop && y <= plotBottom) {
      display->drawPixel(i, y, SH110X_WHITE);
    }

    if (sampleVolts < vMin) vMin = sampleVolts;
    if (sampleVolts > vMax) vMax = sampleVolts;
  }

  return vMax - vMin;
}

void ScopeApp::applyConfiguredSampleRate(UIContext& ctx) const {
  uint16_t sampleSize = ctx.dsp->getSampleSize();
  uint16_t displaySamples = computeDisplaySamples(sampleSize);
  uint32_t sampleRate = computeScopeSampleRate(displaySamples, _time.values[_timeIndex]);
  ctx.dsp->changeSampleRate(sampleRate);
}

void ScopeApp::draw(UIContext& ctx, bool fullRedraw) {
  processInput(ctx);
  auto d = ctx.display;
  auto dsp = ctx.dsp;
  const uint8_t PLOT_TOP = 10;
  const uint8_t PLOT_BOTTOM = 63;
  const uint8_t PLOT_HEIGHT = 54;
  const uint8_t PLOT_CENTER = 37;
  const float voltsPerRange = _amplitude.values[_amplitudeIndex];
  const float voltsToPixels = (PLOT_HEIGHT * 0.5f) / voltsPerRange;

  if (fullRedraw)
    d->clearDisplay();
  else
    d->fillRect(0, PLOT_TOP, 128, PLOT_HEIGHT, SH110X_BLACK);
  drawUI(ctx);
  uint16_t sampleSize = dsp->getSampleSize();
  const uint16_t plotWidth = 128;
  const bool showBothChannels = (_channelIndex == 2);
  float* waveformVolts0 = dsp->getWaveformVolts(0);
  const bool needChannel2 = showBothChannels || (_channelIndex == 1);
  float* waveformVolts1 = needChannel2 ? dsp->getWaveformVolts(1) : nullptr;
  float* activeWaveform = (_channelIndex % 2 == 0) ? waveformVolts0 : waveformVolts1;
  float* triggerWaveform = showBothChannels ? waveformVolts0 : activeWaveform;

  if (!triggerWaveform || sampleSize == 0) {
    d->display();
    return;
  }

  uint16_t displaySamples = computeDisplaySamples(sampleSize);
  uint16_t triggerSearchEndExclusive = 0;
  if (sampleSize > displaySamples) {
    triggerSearchEndExclusive = (sampleSize - displaySamples) + 1;
  }

  uint16_t plotStart = findFirstRisingZeroCrossing(triggerWaveform, triggerSearchEndExclusive);

  float vpp0 = 0.0f;
  float vpp1 = 0.0f;
  if (showBothChannels) {
    vpp0 = drawScopeTrace(d, waveformVolts0, sampleSize, plotStart, displaySamples, plotWidth,
                          PLOT_TOP, PLOT_BOTTOM, PLOT_CENTER, voltsToPixels);
    vpp1 = drawScopeTrace(d, waveformVolts1, sampleSize, plotStart, displaySamples, plotWidth,
                          PLOT_TOP, PLOT_BOTTOM, PLOT_CENTER, voltsToPixels);
  } else {
    float vpp = drawScopeTrace(d, activeWaveform, sampleSize, plotStart, displaySamples, plotWidth,
                               PLOT_TOP, PLOT_BOTTOM, PLOT_CENTER, voltsToPixels);
    d->setCursor(0, 50);
    d->printf("Vpp: %.2fV", vpp);
  }

  if (showBothChannels) {
    d->setCursor(0, 50);
    d->printf("1:%.2f 2:%.2f", vpp0, vpp1);
  }
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
          applyConfiguredSampleRate(ctx);
          break;
      }
    }
    if (ctx.buttons->shortPressed(BACK_BTN) || ctx.buttons->shortPressed(ENC_BTN)) {
      _editing = false;
    }
  }
}
