#pragma once

#include <Arduino.h>
#include <hardware/adc.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <pico/stdlib.h>
#define ARM_MATH_CM0PLUS
#include "arm_math.h"
#include <math.h>

#define DSP_MAX_SAMPLES 1024
#define ADC_REF 20.0f  // 20v pp
//#define ADC_SCALE_1 0.1581f // Initial voltage divider scaling
#define ADC_MAX 4095.0f
#define ADC_OFFSET (ADC_MAX * 47.0f / 100.0f)  // Offset
#define NUM_DMA_BUFFERS 2

class DSPChannel {
public:
  DSPChannel(uint8_t adcPin, uint16_t sampleSize, uint32_t sampleRate);
  void begin();
  volatile int _currentBuffer = 0;
  uint16_t _buffers[NUM_DMA_BUFFERS][DSP_MAX_SAMPLES];
  volatile bool _bufferReady = false;

  
  // Voltmeter
  float getVoltageDC();
  float getVoltageRMS();
  float getVoltageMin();
  float getVoltageMax();

  // Scope
  uint16_t* getWaveform();
  float* getWaveformFloat();

  // FFT
  float* getSpectrum();
  float getFundamentalFrequency();
  void computeFFT();
  void processBuffer(uint16_t* processBufferPtr);
  void changeSampleSize(uint16_t newSampleSize);

  int _dmaChannel;
  
  uint16_t _sampleSize;
  uint16_t* _activeBuffer;  // Points to current buffer for processing
private:
  void detectFundamental();
  float computeHPSFrequency();
  
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
  uint32_t _sampleRate;


  // FFT instance
  arm_cfft_instance_f32 _cfft;

};
