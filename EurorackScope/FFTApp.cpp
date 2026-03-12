#include <Arduino.h>
#include "FFTApp.h"
#include <math.h>
#include "App.h"
#include "Menu.h"
FFTApp::FFTApp(const Menu* menu)
: App(menu)
{
}

void FFTApp::draw(UIContext& ctx, bool fullRedraw)
{
  auto d = ctx.display;
  auto dsp = ctx.dsp;

    dsp->getChannel(0)->computeFFT();
  uint8_t width = d->width();
  const uint8_t BOTTOM = d->height() - 2;
  uint8_t step = DSP_MAX_SAMPLES / width;
  const float SCALE = 1.0;
  const uint8_t SAMPLING_INTERVAL = DSP_MAX_SAMPLES / width;
  d->clearDisplay();
  d->setTextSize(1);
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  d->setCursor(36, 0);
  d->println("   FFT");

  float* buffer = dsp->getChannel(0)->getSpectrum();

  for (uint16_t i = 0; i < width; i++) {
    uint8_t y = constrain(floor(BOTTOM - buffer[i * SAMPLING_INTERVAL] * SCALE), 9, BOTTOM);

    d->drawPixel(i, y, SH110X_WHITE);
  }
  d->display();
}
