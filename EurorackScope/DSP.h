#pragma once

#include <Arduino.h>
#include <hardware/adc.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <pico/stdlib.h>
#define ARM_MATH_CM0PLUS
#include "arm_math.h"
#include "DSPChannel.h"
#include <math.h>

#define DSP_MAX_SAMPLES 1024



class DSP {
public:
  DSP(uint8_t adcPin1, uint8_t adcPin2);

  void begin(uint32_t sampleRate, uint16_t sampleSize);
  void update();  // non-blocking

  uint16_t getSampleSize();
  void changeSampleSize(uint16_t newSampleSize);
  void changeSampleRate(uint32_t newSampleRate);
  DSPChannel* getChannel(uint8_t ch);


private:
  // Core functionality
  void setupADC(uint8_t adcPin);
  void setupDMA(uint8_t channel);
  
  void stopAcquisition();
  void reconfigureADC();
  void restartAcquisition();
  // Hardware
  uint8_t _adcPin1, _adcPin2;


  // Sample configuration
  uint16_t _sampleSize;
  uint32_t _sampleRate;

  DSPChannel * _channels[2];
  uint8_t _activeChannel;
 
  // Timing
  uint32_t _lastProcess = 0;
};

// Global instance pointer for interrupt handler
static DSP* instancePtr = nullptr;