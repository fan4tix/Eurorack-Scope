#pragma once

#include <Arduino.h>
#include <hardware/adc.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <pico/stdlib.h>
#define ARM_MATH_CM0PLUS
#include "arm_math.h"
#include <math.h>

#define DSP_MAX_SAMPLES 2048
#define ADC_MAX_COUNTS 4095.0f
#define ADC_VREF_VOLTS 3.3f
#define ADC_INPUT_OFFSET_VOLTS 1.4942857f
#define ADC_INPUT_DIVIDER_GAIN 0.022142848f
#define NUM_DMA_BUFFERS 2

class DSPChannel {
public:
  DSPChannel();
  void begin(uint8_t adcPin, uint16_t sampleSize, uint32_t sampleRate, uint16_t* buffer);
  volatile int _currentBuffer = 0;

  volatile bool _bufferReady = false;


  // Voltmeter
  float getVoltageDC();
  float getVoltageRMS();
  float getVoltageMin();
  float getVoltageMax();

  // Scope
  uint16_t* getWaveform();
  float* getWaveformFloat();
  float* getWaveformVolts();

  // FFT
  float* getSpectrum();
  float getFundamentalFrequency();
  void computeFFT();
  void processBuffer();
  void setSampleRate(uint32_t sampleRate);




private:
  void detectFundamental();
  float computeHPSFrequency();
  float adcCountToInputVolts(uint16_t adcCount) const;
  uint16_t* _activeBuffer;  // Points to current buffer for processing

  uint32_t _sampleRate;
  uint16_t _sampleSize;

  uint8_t _adcPin;
  // DMA Buffers
  // Processing buffers
  float _fbuffer[DSP_MAX_SAMPLES];
  float _fftBuffer[2 * DSP_MAX_SAMPLES];  // Complex interleaved
  float _spectrum[DSP_MAX_SAMPLES / 2];
  float _window[DSP_MAX_SAMPLES];
  // Results
  float _dc;
  float _rms;
  float _vMin;
  float _vMax;
  float _fundamentalFreq;

  // FFT instance
  arm_cfft_instance_f32 _cfft;
};
