#pragma once
#include "App.h"

class FFTApp : public App {
public:
    FFTApp(const Menu* menu);

    void draw(UIContext& ctx, bool fullRedraw) override;

private:
    bool _gridDrawn = false;
    uint8_t _activeChannel = 0;
};