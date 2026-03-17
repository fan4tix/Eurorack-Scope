
#include <Arduino.h>
#include "LissajousApp.h"
#include <math.h>
#include "App.h"
#include "Menu.h"
LissajousApp::LissajousApp(const Menu* menu)
: App(menu)
{
}

void LissajousApp::draw(UIContext& ctx, bool fullRedraw)
{
   
  const uint8_t CENTER_Y = 37;
  
  const uint8_t CENTER_X = 63;
  
  const float RANGE = .1f;
  auto d = ctx.display;
  auto dsp = ctx.dsp;
  d->clearDisplay();
d->setTextSize(1);
    d->setTextColor(SH110X_WHITE, SH110X_BLACK);
    d->setCursor(36, 0);
    d->println("Lissajous");
    
  float* buffer0 = dsp->getChannel(0)->getWaveformFloat();
  float* buffer1 = dsp->getChannel(1)->getWaveformFloat();

  for (uint16_t i = 0; i < 128; i++)
    d->drawPixel(floor(CENTER_X - buffer0[i] * RANGE / 2),
                 floor(CENTER_Y - buffer1[i] * RANGE / 2),
                 SH110X_WHITE);
  d->display();
}

