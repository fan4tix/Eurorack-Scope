#include <Arduino.h>
#include "TunerApp.h"
#include <math.h>
#include <stdio.h>
#include "App.h"
#include "Menu.h"

static const uint32_t kTunerSampleRateHz = 4096;

static void frequencyToNearestNote(float frequencyHz, char* outNote, size_t outLen, float* outRefHz, float* outCents) {
  static const char* kNoteNames[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
  };

  if (frequencyHz <= 0.0f) {
    snprintf(outNote, outLen, "--");
    *outRefHz = 0.0f;
    *outCents = 0.0f;
    return;
  }

  float midiFloat = 69.0f + 12.0f * log2f(frequencyHz / 440.0f);
  int32_t midiNote = (int32_t)lroundf(midiFloat);
  if (midiNote < 0) {
    midiNote = 0;
  }
  if (midiNote > 127) {
    midiNote = 127;
  }

  uint8_t noteIndex = (uint8_t)(midiNote % 12);
  int8_t octave = (int8_t)(midiNote / 12) - 1;
  *outRefHz = 440.0f * powf(2.0f, (midiNote - 69) / 12.0f);
  *outCents = 1200.0f * log2f(frequencyHz / *outRefHz);
  snprintf(outNote, outLen, "%s%d", kNoteNames[noteIndex], octave);
}

TunerApp::TunerApp(const Menu* menu)
  : App(menu) {
  resetPitchHistory();
}

void TunerApp::resetPitchHistory() {
  _pitchHistoryCount = 0;
  _pitchHistoryIndex = 0;
  _lastStablePitchHz = 0.0f;
  for (uint8_t i = 0; i < kPitchAvgWindow; ++i) {
    _pitchHistory[i] = 0.0f;
  }
}

void TunerApp::onEnter(UIContext& ctx) {
  if (ctx.dsp->getSampleRate() != kTunerSampleRateHz) {
    ctx.dsp->changeSampleRate(kTunerSampleRateHz);
  }
  resetPitchHistory();
}

void TunerApp::draw(UIContext& ctx, bool fullRedraw) {
  (void)fullRedraw;
  auto d = ctx.display;
  auto dsp = ctx.dsp;

  _activeChannel += ctx.encoderDelta;
  _activeChannel %= 2;

  DSPChannel* channel = dsp->getChannel(_activeChannel);
  if (!channel) {
    return;
  }

  channel->computeFFT();

  float rawPitchHz = channel->getFundamentalFrequency();

  if (rawPitchHz > 0.0f) {
    _pitchHistory[_pitchHistoryIndex] = rawPitchHz;
    _pitchHistoryIndex = (_pitchHistoryIndex + 1) % kPitchAvgWindow;
    if (_pitchHistoryCount < kPitchAvgWindow) {
      _pitchHistoryCount++;
    }

    float sum = 0.0f;
    for (uint8_t i = 0; i < _pitchHistoryCount; ++i) {
      sum += _pitchHistory[i];
    }
    _lastStablePitchHz = sum / _pitchHistoryCount;
  }

  float filteredPitchHz = _lastStablePitchHz;

  char noteLabel[8];
  float noteRefHz = 0.0f;
  float centsOffset = 0.0f;
  frequencyToNearestNote(filteredPitchHz, noteLabel, sizeof(noteLabel), &noteRefHz, &centsOffset);

  d->clearDisplay();

  d->setTextSize(1);
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  d->setCursor(32, 0);
  d->printf("Tuner (CH%d)\n", _activeChannel + 1);

  const int16_t barCenter = 60;
  const int16_t barHalfWidth = 44;
  const int16_t barLeft = barCenter - barHalfWidth;
  const int16_t barRight = barCenter + barHalfWidth;
  const int16_t barY = 50;
  const int16_t arrowTop = 42;
  const int16_t centsRange = 50;

  for (uint8_t i = barLeft; i <= barRight; i += 8) {
    d->drawLine(i, barY, i, barY + TUNER_LINE_HEIGHT, SH110X_WHITE);
  }
  d->drawLine(barCenter, barY, barCenter, barY + TUNER_LINE_HEIGHT + 3, SH110X_WHITE);

  float centsClamped = centsOffset;
  if (centsClamped > centsRange) centsClamped = centsRange;
  if (centsClamped < -centsRange) centsClamped = -centsRange;
  float norm = centsClamped / centsRange;
  int16_t arrowX = barCenter + (int16_t)lroundf(norm * (barRight - barCenter));
  if (arrowX < barLeft) arrowX = barLeft;
  if (arrowX > barRight) arrowX = barRight;

  //d->drawLine(arrowX, arrowTop, arrowX, barY - 1, SH110X_WHITE);
  d->drawTriangle(arrowX - 3, arrowTop, arrowX + 3, arrowTop, arrowX, arrowTop + 4, SH110X_WHITE);

  d->setTextSize(2);
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  d->setCursor(42, 12);
  d->print(noteLabel);

  d->setTextSize(1);
  d->setCursor(0, 32);
  d->printf("%5.1fHz", filteredPitchHz);
  d->setCursor(74, 32);
  d->printf("%+.1fc", centsOffset);

  d->display();
}
