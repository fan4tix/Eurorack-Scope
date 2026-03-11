#pragma once
#include "App.h"

struct UIOption {
  const uint8_t len;
  const char** names;
  const float* values;
};
// arrays
static const char* TIME_NAMES[] = { "1s", ".5s", ".25s", ".1s", "50ms", "25ms", "10ms", "5ms", "2.5ms", "1ms" };
static const float TIME_VALUES[] = { 1000, 500, 250, 100, 50, 25, 10, 5, 2.5, 1 };

static const char* AMP_NAMES[] = { "10", "5", "2", "1", ".5", ".2", ".1" };
static const float AMP_VALUES[] = { 10, 5, 2, 1, .5, .2, .1 };

static const char* CH_NAMES[] = { "1", "2", "1+2" };
static const float CH_VALUES[] = { 1, 2, 0 };

#define SCOPE_APP_MAX_UI_ITEMS 3
class ScopeApp : public App {
public:
  ScopeApp(const Menu* menu);

  void draw(UIContext& ctx, bool fullRedraw) override;

private:
  void drawUI(UIContext& ctx);
  void processInput(UIContext& ctx);
  bool _gridDrawn = false;

  uint8_t _selectionIndex = 0;
  bool _editing = false;
  // arrays
  const UIOption _time{ 10, TIME_NAMES, TIME_VALUES };
  const UIOption _amplitude{ 7, AMP_NAMES, AMP_VALUES };
  const UIOption _channel{ 3, CH_NAMES, CH_VALUES };
  uint8_t _timeIndex = 0, _amplitudeIndex = 0, _channelIndex = 0;
};