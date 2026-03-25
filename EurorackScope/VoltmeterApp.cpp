
#include <Arduino.h>
#include "VoltmeterApp.h"
#include <math.h>
#include "App.h"
#include "Menu.h"

// Target ~30 Hz update rate: sampleRate = sampleSize × fps = 2048 × 30
static const uint32_t kVoltmeterSampleRateHz = 61440;

VoltmeterApp::VoltmeterApp(const Menu* menu)
  : App(menu) {
}

void VoltmeterApp::onEnter(UIContext& ctx) {
  ctx.dsp->changeSampleRate(kVoltmeterSampleRateHz);
}

void VoltmeterApp::draw(UIContext& ctx, bool fullRedraw) {
  auto d = ctx.display;
  auto dsp = ctx.dsp;
  if (fullRedraw) {
    d->clearDisplay();
    d->setTextSize(1);
    d->setTextColor(SH110X_WHITE, SH110X_BLACK);
    d->setCursor(18, 12);
    d->print("CH1:");
    d->setCursor(78, 12);
    d->print("CH2:");
    d->drawLine(60, 12, 60, 60, SH110X_WHITE);
    d->setTextSize(1);
    d->setTextColor(SH110X_WHITE, SH110X_BLACK);
    d->setCursor(36, 0);
    d->println("Voltmeter");
  }
  d->setTextSize(1);
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  d->setCursor(0, 46);
  d->printf("Min:%2.2fV ", dsp->getChannel(0)->getVoltageMin());
  d->setCursor(0, 55);
  d->printf("Max:%2.2fV ", dsp->getChannel(0)->getVoltageMax());
  d->setCursor(64, 46);
  d->printf("Min:%2.2fV ", dsp->getChannel(1)->getVoltageMin());
  d->setCursor(64, 55);
  d->printf("Max:%2.2fV ", dsp->getChannel(1)->getVoltageMax());

  d->setTextSize(2);
  d->setCursor(0, 24);
  d->printf("%.2f ", dsp->getChannel(0)->getVoltageDC());

  d->setCursor(64, 24);
  d->printf("%.2f ", dsp->getChannel(1)->getVoltageDC());

  d->display();
}
