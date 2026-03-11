#pragma once
#include "App.h"

class PSUApp : public App {
public:
    PSUApp(const Menu* menu);

    void draw(UIContext& ctx, bool fullRedraw) override;

private:
    bool _gridDrawn = false;
};