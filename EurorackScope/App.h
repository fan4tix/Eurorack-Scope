#pragma once

#include "UIContext.h"
#include "Menu.h"

class App {
public:
    App(const Menu* rootMenu)
        : _menu(rootMenu) {}

    virtual ~App() {}

    // lifecycle
    virtual void onEnter(UIContext& ctx) {}
    virtual void onExit(UIContext& ctx) {}

    // main rendering
    virtual void draw(UIContext& ctx, bool fullRedraw) = 0;

    // optional update hook
    virtual void update(UIContext& ctx) {}

    // shared menu access
    const Menu* getMenu() const {
        return _menu;
    }

protected:
    const Menu* _menu;
};