#pragma once
#include "App.h"

#define TUNER_LINE_HEIGHT 3

class TunerApp : public App {
public:
    TunerApp(const Menu* menu);

    void draw(UIContext& ctx, bool fullRedraw) override;

private:
    bool _gridDrawn = false;
    uint8_t _activeChannel = 0;
};