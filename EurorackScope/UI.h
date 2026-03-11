#pragma once

#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

struct App;
struct Menu;


#include "PSU.h"
#include "Buttons.h"
#include "DSP.h"
#include "UIContext.h"
#define LINE_HEIGHT 9


// --------------------------------------------------
class UI {
public:
  UI(UIContext* ctx,
     App** apps,
     uint8_t appCount);
  void begin();

  uint8_t update(int8_t encoderDelta);

  void back();
  void changeMode(uint8_t id);
private:

  void drawMenu();
  void updateSelection();
  void updateValue();
  void enter(const Menu* menu);
  void openMenu();
  void drawApp(bool fullRedraw);



  Adafruit_SH1106G* _d;

  // Menu
  const Menu* _currentMenu;
  const Menu* _stack[5];
  uint8_t _depth;
  int8_t _index;
  bool _editing;
  bool _inMenu = false;

  // Apps
  App** _apps;
  uint8_t _appCount;
  App* _currentApp;
  UIContext* _ctx;
};
