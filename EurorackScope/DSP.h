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

class DSP {
public:
  DSP(uint8_t adcPin);

  void begin(uint32_t sampleRate, uint16_t sampleSize);
  void update();  // non-blocking

  // Voltmeter
  float getVoltageDC();
  float getVoltageRMS();
  float getVoltageMin();
  float getVoltageMax();

  // Scope
  uint16_t* getWaveform();
  float* getWaveformFloat();
  uint16_t getSampleSize();
  void changeSampleSize(uint16_t newSampleSize);
  void changeSampleRate(uint32_t newSampleRate);

  // FFT
  float* getSpectrum();
  float getFundamentalFrequency();
  void computeFFT();

  int _dmaChannel;
  volatile int _currentBuffer = 0;
  uint16_t _buffers[NUM_DMA_BUFFERS][DSP_MAX_SAMPLES];
  volatile bool _bufferReady = false;

private:
  // Core functionality
  void setupADC();
  void setupDMA();
  void processBuffer(uint16_t* processBufferPtr);
  void detectFundamental();
  float computeHPSFrequency();
  void stopAcquisition();
  void reconfigureADC();
  void restartAcquisition();
  // Hardware
  uint8_t _adcPin;


  // DMA Buffers
  uint16_t* _activeBuffer;  // Points to current buffer for processing

  // Sample configuration
  uint16_t _sampleSize;
  uint32_t _sampleRate;

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

  // Timing
  uint32_t _lastProcess = 0;
};

// Global instance pointer for interrupt handler
static DSP* instancePtr = nullptr;