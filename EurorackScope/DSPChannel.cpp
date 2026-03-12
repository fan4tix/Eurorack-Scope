#include "DSPChannel.h"


DSPChannel::DSPChannel(uint8_t adcPin, uint16_t sampleSize, uint32_t sampleRate)
  : _adcPin(adcPin), _sampleSize(sampleSize), _sampleRate(sampleRate), _rms(0.0f), _dc(0.0f), _vMin(0.0f), _vMax(0.0f), _fundamentalFreq(0.0f), _currentBuffer(0), _bufferReady(false) {}

void DSPChannel::begin() {
  // Initialize active buffer pointer
  _activeBuffer = _buffers[0];

  // Initialize FFT
  arm_cfft_init_f32(&_cfft, _sampleSize);

  // Initialize Hann window
  for (uint16_t i = 0; i < _sampleSize; i++) {
    _window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (_sampleSize - 1)));
  }
}

void DSPChannel::changeSampleSize(uint16_t newSampleSize) {
  // Ensure power of 2 for FFT
  if (newSampleSize & (newSampleSize - 1)) {
    while (newSampleSize & (newSampleSize - 1)) {
      newSampleSize--;
    }
  }

  if (newSampleSize == _sampleSize) return;
  if (newSampleSize > DSP_MAX_SAMPLES) newSampleSize = DSP_MAX_SAMPLES;


  // Update size
  _sampleSize = newSampleSize;

  // Reinitialize FFT for new size
  arm_cfft_init_f32(&_cfft, _sampleSize);

  // Recompute window for new size
  for (uint16_t i = 0; i < _sampleSize; i++) {
    _window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (_sampleSize - 1)));
  }

}
//=============================================================================
// Getters & Setters
//=============================================================================
float DSPChannel::getFundamentalFrequency() {
  return _fundamentalFreq;
}
float DSPChannel::getVoltageDC() {
  return _dc;
}
float DSPChannel::getVoltageRMS() {
  return _rms;
}
float DSPChannel::getVoltageMin() {
  return _vMin;
}
float DSPChannel::getVoltageMax() {
  return _vMax;
}

uint16_t* DSPChannel::getWaveform() {
  return _activeBuffer;
}
float* DSPChannel::getWaveformFloat() {
  return _fbuffer;
}
float* DSPChannel::getSpectrum() {
  return _spectrum;
}


//=============================================================================
// FFT Processing
//=============================================================================
void DSPChannel::computeFFT() {
  // Prepare complex input
  for (uint16_t i = 0; i < _sampleSize; i++) {
    float v = (_activeBuffer[i] / ADC_MAX) * ADC_REF;
    v -= _dc;         // Remove DC
    v *= _window[i];  // Apply Hann window

    _fftBuffer[2 * i] = v;         // Real part
    _fftBuffer[2 * i + 1] = 0.0f;  // Imaginary part
  }

  // Execute FFT
  arm_cfft_f32(&_cfft, _fftBuffer, 0, 1);

  // Compute magnitude spectrum
  for (uint16_t i = 0; i < _sampleSize / 2; i++) {
    float real = _fftBuffer[2 * i];
    float imag = _fftBuffer[2 * i + 1];
    float mag = sqrtf(real * real + imag * imag);
    _spectrum[i] = mag * 2.0f / _sampleSize;  // Normalize
  }

  _fundamentalFreq = computeHPSFrequency();
}

//=============================================================================
// Fundamental Frequency Detection
//=============================================================================
void DSPChannel::detectFundamental() {
  uint16_t startBin = 2;
  uint16_t endBin = _sampleSize / 2 - 2;

  float maxVal = 0.0f;
  uint16_t maxIndex = 0;

  // Find peak magnitude
  for (uint16_t i = startBin; i < endBin; i++) {
    if (_spectrum[i] > maxVal) {
      maxVal = _spectrum[i];
      maxIndex = i;
    }
  }

  // Signal too weak
  if (maxVal < 0.001f) {
    _fundamentalFreq = 0.0f;
    return;
  }

  // Parabolic interpolation
  float alpha = _spectrum[maxIndex - 1];
  float beta = _spectrum[maxIndex];
  float gamma = _spectrum[maxIndex + 1];
  float denom = (alpha - 2.0f * beta + gamma);

  float peakOffset = 0.0f;
  if (fabsf(denom) > 1e-6f) {
    peakOffset = 0.5f * (alpha - gamma) / denom;
  }

  float trueIndex = maxIndex + peakOffset;
  float binWidth = (float)_sampleRate / _sampleSize;
  _fundamentalFreq = trueIndex * binWidth;
}

float DSPChannel::computeHPSFrequency() {
  const uint16_t N = _sampleSize / 2;
  const float binWidth = (float)_sampleRate / _sampleSize;
  const uint8_t HPS_LEVELS = 4;

  float maxValue = 0.0f;
  uint16_t maxIndex = 0;

  uint16_t startBin = 2;
  uint16_t endBin = min((uint16_t)(5000.0f / binWidth), N / HPS_LEVELS);

  for (uint16_t i = startBin; i < endBin; i++) {
    float hps = _spectrum[i];

    // Harmonic compression
    for (uint8_t h = 2; h <= HPS_LEVELS; h++) {
      hps *= _spectrum[i * h];
    }

    if (hps > maxValue) {
      maxValue = hps;
      maxIndex = i;
    }
  }

  // Quadratic interpolation
  if (maxIndex > 1 && maxIndex < N - 1) {
    float y0 = _spectrum[maxIndex - 1];
    float y1 = _spectrum[maxIndex];
    float y2 = _spectrum[maxIndex + 1];
    float denom = (y0 - 2.0f * y1 + y2);

    if (fabsf(denom) > 1e-6f) {
      float delta = 0.5f * (y0 - y2) / denom;
      return (maxIndex + delta) * binWidth;
    }
  }

  return maxIndex * binWidth;
}


//=============================================================================
// Buffer Processing
//=============================================================================
void DSPChannel::processBuffer(uint16_t* processBufferPtr) {
  float sum = 0.0f;
  float sumSq = 0.0f;
  _vMin = 1000.0f;
  _vMax = -1000.0f;

  for (uint16_t i = 0; i < _sampleSize; i++) {
    float v = ((processBufferPtr[i] - ADC_OFFSET) / ADC_MAX) * ADC_REF;
    _fbuffer[i] = v;
    sum += v;

    if (v < _vMin) _vMin = v;
    if (v > _vMax) _vMax = v;

    sumSq += v * v;
  }

  _rms = sqrtf(sumSq / _sampleSize);
  _dc = sum / _sampleSize;
}
