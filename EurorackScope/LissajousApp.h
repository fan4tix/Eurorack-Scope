#pragma once
#include "App.h"

struct LissajousOption {
  const uint8_t len;
  const char** names;
  const float* values;
};

static const char* LISSA_TIME_NAMES[] = { "1s", ".5s", ".25s", ".1s", "50ms", "25ms", "10ms", "5ms" };
static const float LISSA_TIME_VALUES[] = { 1000, 500, 250, 100, 50, 25, 10, 5 };

static const char* LISSA_AMP_NAMES[] = { "10", "5", "2", "1", ".5", ".2", ".1" };
static const float LISSA_AMP_VALUES[] = { 10, 5, 2, 1, .5, .2, .1 };

#define LISSA_APP_MAX_UI_ITEMS 2

class LissajousApp : public App {
public:
    LissajousApp(const Menu* menu);
    void onEnter(UIContext& ctx) override;

    void draw(UIContext& ctx, bool fullRedraw) override;

private:
    uint32_t computeSampleRate(uint16_t sampleSize, float windowMs) const;
    void applyConfiguredSampleRate(UIContext& ctx) const;
    void drawOverlayUI(UIContext& ctx);
    void processInput(UIContext& ctx);
    void markUIActivity();
    bool shouldShowUI() const;

    bool _gridDrawn = false;
    uint8_t _selectionIndex = 0;
    bool _editing = false;
    uint32_t _uiVisibleUntilMs = 0;
    static const uint32_t kUIVisibleMs = 2000;

    const LissajousOption _time{ 8, LISSA_TIME_NAMES, LISSA_TIME_VALUES };
    const LissajousOption _amplitude{ 7, LISSA_AMP_NAMES, LISSA_AMP_VALUES };
    uint8_t _timeIndex = 0;
    uint8_t _amplitudeIndex = 3;
};