#pragma once
#include "App.h"

class VoltmeterApp : public App {
public:
    VoltmeterApp(const Menu* menu);

    void draw(UIContext& ctx, bool fullRedraw) override;

private:
    bool _gridDrawn = false;
};