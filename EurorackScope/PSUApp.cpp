
#include <Arduino.h>
#include "PSUApp.h"
#include <math.h>
#include "App.h"
#include "Menu.h"
PSUApp::PSUApp(const Menu* menu)
  : App(menu) {
}

void PSUApp::draw(UIContext& ctx, bool fullRedraw) {
  static int8_t lastScreen = -1;

  if (_screen != lastScreen) {
    fullRedraw = true;
    lastScreen = _screen;
  }

  auto d = ctx.display;
  auto psu = ctx.psu;

  _screen += ctx.encoderDelta;

  _screen %= 2;

  psu->update();
  const uint8_t COL1 = 0, COL2 = 35, COL3 = 80, OFFSET = 5;
  if (fullRedraw) {
    d->clearDisplay();
    d->setTextSize(1);
    d->setTextColor(SH110X_WHITE, SH110X_BLACK);
    d->setCursor(36, 0);
    if (_screen == 0)
      d->println("Voltages");
    else
      d->println("Currents");

    // Draw Legend:
    d->setCursor(COL1, 2 * LINE_HEIGHT);
    d->print("Rail");

    d->setCursor(COL2, 2 * LINE_HEIGHT);
    d->print(" PSU A");

    d->setCursor(COL3, 2 * LINE_HEIGHT);
    d->print(" PSU B");

    d->setCursor(COL1, OFFSET + 3 * LINE_HEIGHT);
    d->print("+5V");
    d->setCursor(COL1, OFFSET + 4 * LINE_HEIGHT);
    d->print("+12V");
    d->setCursor(COL1, OFFSET + 5 * LINE_HEIGHT);
    d->print("-12V");

    d->drawLine(COL1, 3 * LINE_HEIGHT, d->width() - 1, 3 * LINE_HEIGHT, SH110X_WHITE);

    d->drawLine(COL2 - OFFSET, 3 * LINE_HEIGHT, COL2 - OFFSET, d->height() - 1, SH110X_WHITE);
  }
  d->setTextSize(1);
  d->setTextColor(SH110X_WHITE, SH110X_BLACK);
  // CH A
  d->setCursor(COL2, OFFSET + 3 * LINE_HEIGHT);
  if (_screen == 0)
    d->printf("% .2fV", psu->getVoltage(PSU::A, PSU::PLUS5));
  else
    d->printf("% .2fA", psu->getCurrent(PSU::A, PSU::PLUS5));

  d->setCursor(COL2, OFFSET + 4 * LINE_HEIGHT);
  if (_screen == 0)
    d->printf("% .2fV", psu->getVoltage(PSU::A, PSU::PLUS12));
  else
    d->printf("% .2fA", psu->getCurrent(PSU::A, PSU::PLUS12));

  d->setCursor(COL2, OFFSET + 5 * LINE_HEIGHT);
  if (_screen == 0)
    d->printf("%.2fV", psu->getVoltage(PSU::A, PSU::MINUS12));
  else
    d->printf("%.2fA", psu->getCurrent(PSU::A, PSU::MINUS12));


  // CH B
  d->setCursor(COL3, OFFSET + 3 * LINE_HEIGHT);
  if (_screen == 0)
    d->printf("% .2fV", psu->getVoltage(PSU::B, PSU::PLUS5));
  else
    d->printf("% .2fA", psu->getCurrent(PSU::B, PSU::PLUS5));

  d->setCursor(COL3, OFFSET + 4 * LINE_HEIGHT);
  if (_screen == 0)
    d->printf("% .2fV", psu->getVoltage(PSU::B, PSU::PLUS12));
  else
    d->printf("% .2fA", psu->getCurrent(PSU::B, PSU::PLUS12));

  d->setCursor(COL3, OFFSET + 5 * LINE_HEIGHT);
  if (_screen == 0)
    d->printf("%.2fV", psu->getVoltage(PSU::B, PSU::MINUS12));
  else
    d->printf("%.2fA", psu->getCurrent(PSU::B, PSU::MINUS12));

  d->display();
}
