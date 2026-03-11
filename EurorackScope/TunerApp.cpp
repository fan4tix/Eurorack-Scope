
#include <Arduino.h>
#include "TunerApp.h"
#include <math.h>
#include "App.h"
#include "Menu.h"




TunerApp::TunerApp(const Menu* menu)
: App(menu)
{
}

void TunerApp::draw(UIContext& ctx, bool fullRedraw)
{
  auto d = ctx.display;
  auto dsp = ctx.dsp;
  dsp->computeFFT();
  if (fullRedraw) {


    d->clearDisplay();

    // Draw Headline
    d->setTextSize(1);
    d->setTextColor(SH110X_WHITE, SH110X_BLACK);
    d->setCursor(46, 0);
    d->println("Tuner");

    // Draw Bar Graph
    for (uint8_t i = 20; i < 108; i += 8) {

      d->drawLine(i, 50, i, 50 + TUNER_LINE_HEIGHT, SH110X_WHITE);
    }
    // Center Line
      d->drawLine(60, 50, 60, 50 + TUNER_LINE_HEIGHT+3, SH110X_WHITE);
  }
  // Draw Note
  d->setTextSize(2);
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  d->setCursor(48, 20);
  d->println("C0");
  d->display();
}

