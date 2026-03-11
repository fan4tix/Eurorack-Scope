#include "DSP.h"

//=============================================================================
// DMA Interrupt Handler
//=============================================================================
uint16_t ctr = 0;
volatile uint16_t last_ctr = 0;
void dma_handler() {
    //Serial.println("int");
  ctr++;
  dma_hw->ints0 = 1u << instancePtr->_dmaChannel;

  // Switch to the other buffer
  int nextBuffer = (instancePtr->_currentBuffer + 1) % NUM_DMA_BUFFERS;
  instancePtr->_currentBuffer = nextBuffer;

  // Immediately reconfigure DMA to use the next buffer
  dma_channel_set_write_addr(instancePtr->_dmaChannel,
                             instancePtr->_buffers[nextBuffer],
                             true);
  // Signal that a buffer is ready for processing
  instancePtr->_bufferReady = true;
}

//=============================================================================
// Constructor
//=============================================================================
DSP::DSP(uint8_t adcPin)
  : _adcPin(adcPin), _sampleSize(512), _sampleRate(20000), _rms(0.0f), _dc(0.0f), _vMin(0.0f), _vMax(0.0f), _fundamentalFreq(0.0f), _lastProcess(0), _currentBuffer(0), _bufferReady(false) {
  instancePtr = this;
}

//=============================================================================
// Initialization
//=============================================================================
void DSP::begin(uint32_t sampleRate, uint16_t sampleSize) {
  _sampleRate = sampleRate;
  _sampleSize = sampleSize;

  // Ensure sample size is power of 2 for FFT
  if (_sampleSize & (_sampleSize - 1)) {
    while (_sampleSize & (_sampleSize - 1)) {
      _sampleSize--;
    }
  }

  // Initialize active buffer pointer
  _activeBuffer = _buffers[0];

  // Initialize FFT
  arm_cfft_init_f32(&_cfft, _sampleSize);

  // Initialize Hann window
  for (uint16_t i = 0; i < _sampleSize; i++) {
    _window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (_sampleSize - 1)));
  }

  // Setup hardware
  setupADC();
  setupDMA();
}

//=============================================================================
// Hardware Setup
//=============================================================================
void DSP::setupADC() {
  adc_init();
  adc_gpio_init(_adcPin);
  adc_select_input(_adcPin - 26);  // GPIO26-29 map to ADC0-3

  adc_fifo_setup(
    true,   // Enable FIFO
    true,   // Enable DMA request
    1,      // Threshold (DREQ when >=1 sample)
    false,  // Don't shift
    false   // Don't use 8-bit mode
  );

  // ADC clock: 48MHz / (96 * divider) = sample rate
  // 96 cycles per sample, 48MHz ADC clock
  float div = 48000000.0f / (_sampleRate * 96.0f);
  adc_set_clkdiv(div);
}

void DSP::setupDMA() {
  // Claim DMA channel
  _dmaChannel = dma_claim_unused_channel(true);
  _currentBuffer = 0;

  // Configure DMA
  dma_channel_config dma_cfg = dma_channel_get_default_config(_dmaChannel);
  channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&dma_cfg, false);
  channel_config_set_write_increment(&dma_cfg, true);
  channel_config_set_dreq(&dma_cfg, DREQ_ADC);

  // Configure with first buffer (don't start yet)
  dma_channel_configure(
    _dmaChannel,
    &dma_cfg,
    _buffers[0],
    &adc_hw->fifo,
    _sampleSize,
    false);

  // Setup interrupt
  irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
  irq_set_enabled(DMA_IRQ_0, true);
  dma_channel_set_irq0_enabled(_dmaChannel, true);

  // Start ADC (runs continuously)
  adc_run(true);

  // Start first DMA transfer
  dma_channel_start(_dmaChannel);
}

//=============================================================================
// Main Update Loop (call frequently)
//=============================================================================
void DSP::update() {
   
  static uint32_t lastDebugTime = 0;
  if (millis() - lastDebugTime > 1000) {
    lastDebugTime = millis();
    
    // Check DMA channel status
    dma_channel_hw_t *hw = dma_channel_hw_addr(_dmaChannel);
    Serial.print("DMA Busy: ");
    Serial.print(dma_channel_is_busy(_dmaChannel));
    Serial.print(" | Transfer count: ");
    Serial.print(hw->transfer_count);
    Serial.print(" | CTR: ");
    Serial.print(ctr);
    Serial.print(" | Buffer ready: ");
    Serial.println(_bufferReady);
  }
  if (!_bufferReady) return;

  // Atomically clear flag and get ready buffer index
  uint32_t saved = save_and_disable_interrupts();
  _bufferReady = false;
  int readyBuffer = (_currentBuffer == 0) ? NUM_DMA_BUFFERS - 1 : _currentBuffer - 1;
  restore_interrupts(saved);

  // Throttle processing to max 100Hz
  if (millis() - _lastProcess > 10) {
    _lastProcess = millis();

    // Process the buffer that's not being written to by DMA
    uint16_t* processBufferPtr = _buffers[readyBuffer];
    processBuffer(processBufferPtr);
  }
}


//=============================================================================
// Sample Rate Change (with minimal disruption)
//=============================================================================
void DSP::changeSampleRate(uint32_t newSampleRate) {
  // Don't do anything if it's the same rate
  if (newSampleRate == _sampleRate) return;
  
  // Store new rate
  _sampleRate = newSampleRate;
  
  // Completely stop acquisition safely
  stopAcquisition();
  
  // Reconfigure ADC with new sample rate
  reconfigureADC();
  
  // Clear any stale data
  adc_fifo_drain();
  
  // Reset DMA and restart
  restartAcquisition();
}

//=============================================================================
// Change sample size
//=============================================================================
void DSP::changeSampleSize(uint16_t newSampleSize) {
  // Ensure power of 2 for FFT
  if (newSampleSize & (newSampleSize - 1)) {
    while (newSampleSize & (newSampleSize - 1)) {
      newSampleSize--;
    }
  }
  
  if (newSampleSize == _sampleSize) return;
  if (newSampleSize > DSP_MAX_SAMPLES) newSampleSize = DSP_MAX_SAMPLES;
  
  // Stop acquisition
  stopAcquisition();
  
  // Update size
  _sampleSize = newSampleSize;
  
  // Reinitialize FFT for new size
  arm_cfft_init_f32(&_cfft, _sampleSize);
  
  // Recompute window for new size
  for (uint16_t i = 0; i < _sampleSize; i++) {
    _window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (_sampleSize - 1)));
  }
  
  // Restart acquisition
  restartAcquisition();
}

//=============================================================================
// Helper methods (add to private section)
//=============================================================================
void DSP::stopAcquisition() {
  // Disable interrupt to prevent handler from firing during reconfig
  dma_channel_set_irq0_enabled(_dmaChannel, false);
  
  // Stop ADC
  adc_run(false);
  
  // Abort DMA and wait
  dma_channel_abort(_dmaChannel);
  while (dma_channel_is_busy(_dmaChannel)) {
    tight_loop_contents();
  }
  
  // Drain FIFO to clear stale data
  adc_fifo_drain();
}

void DSP::reconfigureADC() {
  // Recalculate divider
  float div = 48000000.0f / (_sampleRate * 96.0f);
  adc_set_clkdiv(div);
  
  // Ensure FIFO is still properly configured (redundant but safe)
  adc_fifo_setup(
    true,   // Enable FIFO
    true,   // Enable DMA request
    1,      // Threshold
    false,  // Don't shift
    false   // Don't use 8-bit mode
  );
}

void DSP::restartAcquisition() {
  // Reset buffer pointers
  _currentBuffer = 0;
  _activeBuffer = _buffers[0];
  _bufferReady = false;
  
  // Get fresh DMA config
  dma_channel_config dma_cfg = dma_channel_get_default_config(_dmaChannel);
  channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&dma_cfg, false);
  channel_config_set_write_increment(&dma_cfg, true);
  channel_config_set_dreq(&dma_cfg, DREQ_ADC);
  
  // Configure with first buffer (start immediately)
  dma_channel_configure(
    _dmaChannel,
    &dma_cfg,
    _buffers[0],
    &adc_hw->fifo,
    _sampleSize,
    true  // Start now
  );
  
  // Clear any pending interrupt
  dma_hw->ints0 = 1u << _dmaChannel;
  
  // Re-enable interrupt
  dma_channel_set_irq0_enabled(_dmaChannel, true);
  
  // Restart ADC
  adc_run(true);
}


//=============================================================================
// Buffer Processing
//=============================================================================
void DSP::processBuffer(uint16_t* processBufferPtr) {
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

//=============================================================================
// FFT Processing
//=============================================================================
void DSP::computeFFT() {
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
void DSP::detectFundamental() {
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

float DSP::computeHPSFrequency() {
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
// Getters & Setters
//=============================================================================
float DSP::getFundamentalFrequency() {
  return _fundamentalFreq;
}
float DSP::getVoltageDC() {
  return _dc;
}
float DSP::getVoltageRMS() {
  return _rms;
}
float DSP::getVoltageMin() {
  return _vMin;
}
float DSP::getVoltageMax() {
  return _vMax;
}

uint16_t* DSP::getWaveform() {
  return _activeBuffer;
}
float* DSP::getWaveformFloat() {
  return _fbuffer;
}
uint16_t DSP::getSampleSize() {
  return _sampleSize;
}
float* DSP::getSpectrum() {
  return _spectrum;
}

/*
#define CAPTURE_CHANNEL 0
#define CAPTURE_DEPTH 1000

uint8_t capture_buf[CAPTURE_DEPTH];

void adcStuff() {
  adc_gpio_init(26 + CAPTURE_CHANNEL);

  adc_init();
  adc_select_input(CAPTURE_CHANNEL);
  adc_fifo_setup(
    true,   // Write each completed conversion to the sample FIFO
    true,   // Enable DMA data request (DREQ)
    1,      // DREQ (and IRQ) asserted when at least 1 sample present
    false,  // We won't see the ERR bit because of 8 bit reads; disable.
    true    // Shift each sample to 8 bits when pushing to FIFO
  );

  // Divisor of 0 -> full speed. Free-running capture with the divider is
  // equivalent to pressing the ADC_CS_START_ONCE button once per `div + 1`
  // cycles (div not necessarily an integer). Each conversion takes 96
  // cycles, so in general you want a divider of 0 (hold down the button
  // continuously) or > 95 (take samples less frequently than 96 cycle
  // intervals). This is all timed by the 48 MHz ADC clock.
  adc_set_clkdiv(0);

  Serial.println("Arming DMA\n");
  sleep_ms(1000);
  // Set up the DMA to start transferring data as soon as it appears in FIFO
  uint dma_chan = dma_claim_unused_channel(true);
  dma_channel_config cfg = dma_channel_get_default_config(dma_chan);

  // Reading from constant address, writing to incrementing byte addresses
  channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
  channel_config_set_read_increment(&cfg, false);
  channel_config_set_write_increment(&cfg, true);

  // Pace transfers based on availability of ADC samples
  channel_config_set_dreq(&cfg, DREQ_ADC);

  dma_channel_configure(dma_chan, &cfg,
                        capture_buf,    // dst
                        &adc_hw->fifo,  // src
                        CAPTURE_DEPTH,  // transfer count
                        true            // start immediately
  );

  Serial.println("Starting capture\n");
  adc_run(true);

  // Once DMA finishes, stop any new conversions from starting, and clean up
  // the FIFO in case the ADC was still mid-conversion.
  dma_channel_wait_for_finish_blocking(dma_chan);
  Serial.println("Capture finished\n");
  adc_run(false);
  adc_fifo_drain();

  // Print samples to stdout so you can display them in pyplot, excel, matlab
  for (int i = 0; i < CAPTURE_DEPTH; ++i) {
    Serial.println(capture_buf[i]);
    if (i % 10 == 9)
      Serial.println("\n");
  }

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);
}

// Test signal for scope and fft
void startSquareWavePWM(uint pin, float frequency) {
  // Set pin to PWM function
  gpio_set_function(pin, GPIO_FUNC_PWM);

  // Get PWM slice connected to this GPIO
  uint slice_num = pwm_gpio_to_slice_num(pin);

  // System clock (normally 125 MHz)
  uint32_t sys_clk = clock_get_hz(clk_sys);

 
 //       PWM frequency formula:
//
  //      f_pwm = sys_clk / (clkdiv * (wrap + 1))
//
  //      We choose:
  //          wrap = 65535 (max resolution)
   //     and compute clkdiv.
    

  uint32_t wrap = 65535;

  float clkdiv = (float)sys_clk / (frequency * (wrap + 1));

  // Clamp divider to HW limits
  if (clkdiv < 1.0f) clkdiv = 1.0f;
  if (clkdiv > 255.0f) clkdiv = 255.0f;

  pwm_set_clkdiv(slice_num, clkdiv);
  pwm_set_wrap(slice_num, wrap);

  // 50% duty cycle → square wave
  pwm_set_gpio_level(pin, wrap / 2);

  // Start PWM
  pwm_set_enabled(slice_num, true);
}*/