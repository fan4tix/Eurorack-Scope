#pragma once
#include "App.h"

static const uint32_t FFT_MAX_FREQ_OPTIONS[] = { 1000, 2000, 4000, 8000, 16000 };
static const char* FFT_MAX_FREQ_NAMES[] = { "1k", "2k", "4k", "8k", "16k" };
static const uint8_t FFT_MAX_FREQ_COUNT = 5;

class FFTApp : public App {
public:
  FFTApp(const Menu* menu);
  void onEnter(UIContext& ctx) override;

  void draw(UIContext& ctx, bool fullRedraw) override;

private:
  bool _gridDrawn = false;
  uint8_t _activeChannel = 0;
  uint8_t _maxFreqIdx = 3;    // default 8 kHz
  bool _editingFreq = false;  // encoder controls freq range when true
};