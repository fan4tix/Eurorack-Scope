#include <Arduino.h>
#include "FFTApp.h"
#include <math.h>
#include "App.h"
#include "Menu.h"

static const uint32_t kFFTSampleRateHz = 32768;

FFTApp::FFTApp(const Menu* menu)
  : App(menu) {
}

void FFTApp::onEnter(UIContext& ctx) {
  if (ctx.dsp->getSampleRate() != kFFTSampleRateHz) {
    ctx.dsp->changeSampleRate(kFFTSampleRateHz);
  }
}

void FFTApp::draw(UIContext& ctx, bool fullRedraw) {
  (void)fullRedraw;
  auto d = ctx.display;
  auto dsp = ctx.dsp;

  // Encoder press toggles between channel select and max-frequency select.
  if (ctx.buttons->shortPressed(ENC_BTN)) {
    _editingFreq = !_editingFreq;
  }
  if (_editingFreq) {
    int8_t next = (int8_t)_maxFreqIdx + ctx.encoderDelta;
    if (next < 0) next = 0;
    if (next >= FFT_MAX_FREQ_COUNT) next = FFT_MAX_FREQ_COUNT - 1;
    _maxFreqIdx = (uint8_t)next;
  } else {
    _activeChannel = (_activeChannel + 2 + ctx.encoderDelta) % 2;
  }

  DSPChannel* channel = dsp->getChannel(_activeChannel);
  if (!channel) {
    return;
  }

  channel->computeFFT();
  uint8_t width = d->width();
  const uint8_t BOTTOM = d->height() - 10;
  const uint8_t TOP = 9;
  const uint16_t sampleSize = dsp->getSampleSize();
  const uint16_t spectrumBins = sampleSize / 2;

  if (spectrumBins < 3) {
    return;
  }

  d->clearDisplay();
  d->setTextSize(1);
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  d->setCursor(0, 0);
  // Header: channel on left, max freq on right (highlighted when editing)
  d->printf("FFT CH%d ", _activeChannel + 1);
  if (_editingFreq)
    d->setTextColor(SH110X_BLACK, SH110X_WHITE);
  d->printf("%sHz", FFT_MAX_FREQ_NAMES[_maxFreqIdx]);
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);

  float* buffer = channel->getSpectrum();
  const uint32_t sampleRate = dsp->getSampleRate();

  // Cap displayed frequency to the user-selected maximum.
  // maxBin is the highest bin whose centre frequency <= maxFreqHz.
  const uint32_t maxFreqHz = FFT_MAX_FREQ_OPTIONS[_maxFreqIdx];
  uint16_t maxBin = (uint16_t)((uint32_t)maxFreqHz * sampleSize / sampleRate);
  if (maxBin < 2)          maxBin = 2;
  if (maxBin >= spectrumBins) maxBin = spectrumBins - 1;

  // Find peak magnitude within the displayed range.
  float maxMag = 0.0f;
  for (uint16_t bin = 1; bin <= maxBin; ++bin) {
    if (buffer[bin] > maxMag) {
      maxMag = buffer[bin];
    }
  }

  // Minimum floor so sub-threshold noise doesn't auto-expand to fill the screen.
  // 0.1 V peak is a practical "no real signal" threshold.
  const float kMinDisplayMag = 0.1f;
  if (maxMag < kMinDisplayMag) {
    maxMag = kMinDisplayMag;
  }

  if (maxMag < 1e-9f) {
    d->setCursor(0, 56);
    d->print("No signal");
    d->display();
    return;
  }

  // Map bins [1, maxBin] evenly across display pixels; each pixel shows the
  // average magnitude of its assigned bins.
  const uint16_t usableBins = maxBin;  // bins 1..maxBin
  for (uint16_t i = 0; i < width; i++) {
    uint16_t binFirst = 1 + ((uint32_t)i       * usableBins) / width;
    uint16_t binLast  = 1 + ((uint32_t)(i + 1) * usableBins) / width;
    if (binLast > maxBin)       binLast = maxBin;
    if (binLast < binFirst)     binLast = binFirst;

    float sum = 0.0f;
    for (uint16_t b = binFirst; b <= binLast; b++) {
      sum += buffer[b];
    }
    float avg = sum / (float)(binLast - binFirst + 1);

    float magNorm = avg / maxMag;
    uint8_t y = constrain((int16_t)floor(BOTTOM - magNorm * (BOTTOM - TOP)), TOP, BOTTOM);
    d->drawLine(i, y, i, BOTTOM, SH110X_WHITE);
  }

  d->setCursor(0, 56);
  d->printf("F:%.1fHz", channel->getFundamentalFrequency());
  d->display();
}
