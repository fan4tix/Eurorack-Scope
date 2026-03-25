#pragma once
#include "App.h"

#define TUNER_LINE_HEIGHT 3

class TunerApp : public App {
public:
  TunerApp(const Menu* menu);
  void onEnter(UIContext& ctx) override;

  void draw(UIContext& ctx, bool fullRedraw) override;

private:
  void resetPitchHistory();
  bool _gridDrawn = false;
  uint8_t _activeChannel = 0;

  static const uint8_t kPitchAvgWindow = 8;
  float _pitchHistory[kPitchAvgWindow];
  uint8_t _pitchHistoryIndex = 0;
  uint8_t _pitchHistoryCount = 0;
  float _lastStablePitchHz = 0.0f;
};