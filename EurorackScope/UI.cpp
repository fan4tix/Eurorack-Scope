#include "UI.h"

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "Buttons.h"
#include "PSU.h"
#include "pinout.h"
#include "Menu.h"
#include "DSP.h"
#include "App.h"
UI::UI(UIContext* ctx,
       App** apps,
       uint8_t appCount)
  : _apps(apps),
    _appCount(appCount),
    _ctx(ctx),
    _currentApp(apps[3]) {


  _currentMenu = _currentApp->getMenu();
  ;
}

void UI::begin() {
  drawApp(true);
}


// --------------------------------------------------
void UI::openMenu() {
  _inMenu = true;
  _stack[0] = _currentMenu;
  _depth = 0;
  _index = 0;
  drawMenu();
}
void UI::enter(const Menu* menu) {

  _stack[_depth++] = _currentMenu;
  _currentMenu = menu;
  _index = 0;
  drawMenu();
}

void UI::back() {
  if (_depth == 0) {
    _inMenu = false;
    drawApp(true);
    return;
  }

  _currentMenu = _stack[--_depth];
  _index = 0;
  drawMenu();
}


// --------------------------------------------------
uint8_t UI::update(int8_t encoderDelta) {
  _ctx->encoderDelta = encoderDelta;
  bool buttonPressed = _ctx->buttons->shortPressed(ENC_BTN);
  bool buttonLongPressed = _ctx->buttons->shortPressed(ENC_BTN);
  bool backPressed = _ctx->buttons->shortPressed(BACK_BTN);

  uint8_t settingsDirty = false;
  if (_inMenu) {
    if (!_currentMenu) return settingsDirty;

    if (backPressed) {
      if (_editing) {
        _editing = false;
        updateValue();
        settingsDirty = true;
      } else {
        back();
      }
    }

    if (_editing) {

      MenuItem& item = (MenuItem&)_currentMenu->items[_index];

      if (item.type == MENU_VALUE) {

        *item.value += encoderDelta * item.step;

        if (*item.value < item.min) *item.value = item.min;
        if (*item.value > item.max) *item.value = item.max;
      }
      if (buttonPressed) {
        _editing = false;
        updateValue();
        settingsDirty = true;
      }


      updateValue();
    } else {

      _index += encoderDelta;

      if (_index < 0) _index = _currentMenu->itemCount - 1;
      if (_index >= _currentMenu->itemCount) _index = 0;

      updateSelection();
      if (buttonPressed) {

        const MenuItem& item = _currentMenu->items[_index];

        switch (item.type) {

          case MENU_SUBMENU:
            enter(item.submenu);

            break;

          case MENU_ACTION:
            item.action();
            break;

          case MENU_VALUE:
            _editing = true;
            updateValue();
            break;
        }
      }
    }



  } else {  // Not in Menu
    if (_ctx->buttons->longPressed(ENC_BTN)) {
      openMenu();
      return settingsDirty;
    } else {
      drawApp(false);
    }
  }
  return settingsDirty;
}

void UI::drawApp(bool fullRedraw) {
  if (_currentApp)
    _currentApp->draw(*_ctx, fullRedraw);
}
void UI::changeMode(uint8_t id) {
  Serial.printf("Switching to %d\n", id);
  if (id >= _appCount) return;
  _currentApp->onExit(*_ctx);
  _currentApp = _apps[id];
  _currentApp->onEnter(*_ctx);
  _currentMenu = _apps[id]->getMenu();
  _depth = 0;
  _inMenu = false;
  Serial.println("Drawing app");
  drawApp(true);
}



// --------------------------------------------------
void UI::drawMenu() {

  _ctx->display->clearDisplay();
  _ctx->display->display();
  // _ctx->display->setTextSize(1);
  _ctx->display->setTextSize(1);
  _ctx->display->setTextColor(SH110X_WHITE, SH110X_BLACK);

  _ctx->display->setCursor(0, 0);

  _ctx->display->println(_currentMenu->title);

  for (uint8_t i = 0; i < _currentMenu->itemCount; i++) {


    _ctx->display->setCursor(0, (1 + i) * LINE_HEIGHT);

    if (i == _index)
      _ctx->display->print("> ");
    else
      _ctx->display->print("  ");


    _ctx->display->print(_currentMenu->items[i].label);


    if (_currentMenu->items[i].type == MENU_VALUE) {


      _ctx->display->print(": ");
      _ctx->display->print(*_currentMenu->items[i].value);
    }
  }


  _ctx->display->display();
}


void UI::updateSelection() {
  for (uint8_t i = 0; i < _currentMenu->itemCount; i++) {

    _ctx->display->setCursor(0, (1 + i) * LINE_HEIGHT);

    if (i == _index)
      _ctx->display->print("> ");
    else
      _ctx->display->print("  ");
  }
  _ctx->display->display();
}

void UI::updateValue() {

  _ctx->display->setCursor(0, (1 + _index) * LINE_HEIGHT);

  if (_editing) {
    _ctx->display->print("->");
  } else {
    _ctx->display->print("> ");
  }

  _ctx->display->print(_currentMenu->items[_index].label);
  _ctx->display->print(": ");
  _ctx->display->print(*_currentMenu->items[_index].value);
  _ctx->display->print(" ");

  _ctx->display->display();
}
