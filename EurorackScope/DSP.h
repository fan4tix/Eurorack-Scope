#pragma once

#include <Arduino.h>
#include <hardware/adc.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <pico/stdlib.h>
#include "DSPChannel.h"
#define DSP_MAX_SAMPLES 2048
#define NUM_DMA_BUFFERS 2

class DSP {

public:
  DSP(uint8_t pin0, uint8_t pin1);

  void begin(uint32_t sampleRate, uint16_t sampleSize);
  void update();

  uint16_t* getWaveform(uint8_t ch);
  float* getWaveformVolts(uint8_t ch);
  uint16_t getSampleSize();
  void changeSampleRate(uint32_t newRate);
  uint32_t getSampleRate();

  DSPChannel* getChannel(uint8_t ch);
private:
  void restartDMA();
  void setupADC();
  void setupDMA();
  void alignADCChannelOrder();
  static void dmaISR();

  uint8_t _pin0;
  uint8_t _pin1;

  uint16_t _sampleSize;
  uint32_t _sampleRate;
  DSPChannel _channels[2];

  int _dma;

  volatile bool _bufferReady;

  uint8_t _currentDMABuffer;

  uint16_t _dmaBuffer[NUM_DMA_BUFFERS][DSP_MAX_SAMPLES * 2];

  uint16_t _ch0[DSP_MAX_SAMPLES];
  uint16_t _ch1[DSP_MAX_SAMPLES];

  static DSP* instance;
};