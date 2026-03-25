#pragma once
#include "App.h"

class VoltmeterApp : public App {
public:
  VoltmeterApp(const Menu* menu);

  void onEnter(UIContext& ctx) override;
  void draw(UIContext& ctx, bool fullRedraw) override;

private:
  bool _gridDrawn = false;
};