#include "DSP.h"

//=============================================================================
// DMA Interrupt Handler
//=============================================================================
uint16_t ctr = 0;
volatile uint16_t last_ctr = 0;
void dma_handler0() {
  //Serial.println("int0");
  ctr++;
  dma_hw->ints0 = 1u << instancePtr->getChannel(0)->_dmaChannel;

  // Switch to the other buffer
  int nextBuffer = (instancePtr->getChannel(0)->_currentBuffer + 1) % NUM_DMA_BUFFERS;
  instancePtr->getChannel(0)->_currentBuffer = nextBuffer;

  // Immediately reconfigure DMA to use the next buffer
  dma_channel_set_write_addr(instancePtr->getChannel(0)->_dmaChannel,
                             instancePtr->getChannel(0)->_buffers[nextBuffer],
                             true);
  // Signal that a buffer is ready for processing
  instancePtr->getChannel(0)->_bufferReady = true;
}

void dma_handler1() {
  //Serial.println("int1");
  dma_hw->ints1 = 1u << instancePtr->getChannel(1)->_dmaChannel;

  // Switch to the other buffer
  int nextBuffer = (instancePtr->getChannel(1)->_currentBuffer + 1) % NUM_DMA_BUFFERS;
  instancePtr->getChannel(1)->_currentBuffer = nextBuffer;

  // Immediately reconfigure DMA to use the next buffer
  dma_channel_set_write_addr(instancePtr->getChannel(1)->_dmaChannel,
                             instancePtr->getChannel(1)->_buffers[nextBuffer],
                             true);
  // Signal that a buffer is ready for processing
  instancePtr->getChannel(1)->_bufferReady = true;
}

//=============================================================================
// Constructor
//=============================================================================
DSP::DSP(uint8_t adcPin1, uint8_t adcPin2)
  : _adcPin1(adcPin1), _adcPin2(adcPin2), _sampleSize(512), _lastProcess(0), _sampleRate(20000) {
  instancePtr = this;

  _channels[0] = new DSPChannel(_adcPin1, _sampleSize, _sampleRate);
  _channels[1] = new DSPChannel(_adcPin2, _sampleSize, _sampleRate);
  _activeChannel = 0;
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



  _channels[0]->begin();
  _channels[1]->begin();



  // Setup hardware
  setupADC(_adcPin1);
  setupADC(_adcPin2);
  setupDMA(0);
  setupDMA(1);

  // Start ADC (runs continuously)
  adc_run(true);

  // Start first DMA transfer
  dma_channel_start(_channels[0]->_dmaChannel);
  dma_channel_start(_channels[1]->_dmaChannel);
}

//=============================================================================
// Hardware Setup
//=============================================================================
void DSP::setupADC(uint8_t adcPin) {
  adc_init();
  adc_gpio_init(adcPin);
  adc_select_input(adcPin - 26);  // GPIO26-29 map to ADC0-3

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

void DSP::setupDMA(uint8_t channel) {
  // Claim DMA channel
  _channels[channel]->_dmaChannel = dma_claim_unused_channel(true);
  _channels[channel]->_currentBuffer = 0;

  // Configure DMA
  dma_channel_config dma_cfg = dma_channel_get_default_config(_channels[channel]->_dmaChannel);
  channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&dma_cfg, false);
  channel_config_set_write_increment(&dma_cfg, true);
  channel_config_set_dreq(&dma_cfg, DREQ_ADC);

  // Configure with first buffer (don't start yet)
  dma_channel_configure(
    _channels[channel]->_dmaChannel,
    &dma_cfg,
    _channels[channel]->_buffers[0],
    &adc_hw->fifo,
    _sampleSize,
    false);

  // Setup interrupt
  irq_set_exclusive_handler(DMA_IRQ_0, channel == 0 ? dma_handler0 : dma_handler1);
  irq_set_enabled(DMA_IRQ_0, true);
  dma_channel_set_irq0_enabled(_channels[channel]->_dmaChannel, true);
}

//=============================================================================
// Main Update Loop (call frequently)
//=============================================================================
void DSP::update() {

  static uint32_t lastDebugTime = 0;
  if (millis() - lastDebugTime > 1000) {
    lastDebugTime = millis();

    // Check DMA channel status
    dma_channel_hw_t* hw = dma_channel_hw_addr(_channels[0]->_dmaChannel);
    Serial.print("DMA Busy: ");
    Serial.print(dma_channel_is_busy(_channels[0]->_dmaChannel));
    Serial.print(" | Transfer count: ");
    Serial.print(hw->transfer_count);
    Serial.print(" | CTR: ");
    Serial.print(ctr);
    Serial.print(" | Buffer ready: ");
    Serial.println(_channels[0]->_bufferReady);
  }
  if (_channels[0]->_bufferReady) {
    // Atomically clear flag and get ready buffer index
    uint32_t saved = save_and_disable_interrupts();
    _channels[0]->_bufferReady = false;
    int readyBuffer = (_channels[0]->_currentBuffer == 0) ? NUM_DMA_BUFFERS - 1 : _channels[0]->_currentBuffer - 1;
    restore_interrupts(saved);

    // Throttle processing to max 100Hz
    if (millis() - _lastProcess > 10) {
      _lastProcess = millis();

      // Process the buffer that's not being written to by DMA
      uint16_t* processBufferPtr = _channels[0]->_buffers[readyBuffer];
      _channels[0]->processBuffer(processBufferPtr);
    }
  }
  if (_channels[1]->_bufferReady) {
    // Atomically clear flag and get ready buffer index
    uint32_t saved = save_and_disable_interrupts();
    _channels[1]->_bufferReady = false;
    int readyBuffer = (_channels[1]->_currentBuffer == 0) ? NUM_DMA_BUFFERS - 1 : _channels[1]->_currentBuffer - 1;
    restore_interrupts(saved);

    // Throttle processing to max 100Hz
    if (millis() - _lastProcess > 10) {  // TODO: LastProcess per channel
      _lastProcess = millis();

      // Process the buffer that's not being written to by DMA
      uint16_t* processBufferPtr = _channels[1]->_buffers[readyBuffer];
      _channels[1]->processBuffer(processBufferPtr);
    }
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

  _channels[0]->changeSampleSize(newSampleSize);
  _channels[1]->changeSampleSize(newSampleSize);

  // Restart acquisition
  restartAcquisition();
}

DSPChannel* DSP::getChannel(uint8_t ch) {
  if (ch > 1) {
    return nullptr;
  }
  return _channels[ch];
}
//=============================================================================
// Helper methods (add to private section)
//=============================================================================
void DSP::stopAcquisition() {
  // Disable interrupt to prevent handler from firing during reconfig
  dma_channel_set_irq0_enabled(_channels[0]->_dmaChannel, false);
  dma_channel_set_irq0_enabled(_channels[1]->_dmaChannel, false);

  // Stop ADC
  adc_run(false);

  // Abort DMA and wait
  dma_channel_abort(_channels[0]->_dmaChannel);
  dma_channel_abort(_channels[1]->_dmaChannel);
  while (dma_channel_is_busy(_channels[0]->_dmaChannel)) {
    tight_loop_contents();
  }
  while (dma_channel_is_busy(_channels[1]->_dmaChannel)) {
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
  for (uint8_t i = 0; i < 2; i++) {

    // Reset buffer pointers
    _channels[i]->_currentBuffer = 0;
    _channels[i]->_activeBuffer = _channels[i]->_buffers[0];
    _channels[i]->_bufferReady = false;

    // Get fresh DMA config
    dma_channel_config dma_cfg = dma_channel_get_default_config(_channels[i]->_dmaChannel);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_cfg, false);
    channel_config_set_write_increment(&dma_cfg, true);
    channel_config_set_dreq(&dma_cfg, DREQ_ADC);

    // Configure with first buffer (start immediately)
    dma_channel_configure(
      _channels[i]->_dmaChannel,
      &dma_cfg,
      _channels[i]->_buffers[0],
      &adc_hw->fifo,
      _channels[i]->_sampleSize,
      true  // Start now
    );

    // Clear any pending interrupt
    if (i == 0) {

      dma_hw->ints0 = 1u << _channels[i]->_dmaChannel;
    } else {

      dma_hw->ints1 = 1u << _channels[i]->_dmaChannel;
    }

    // Re-enable interrupt
    dma_channel_set_irq0_enabled(_channels[i]->_dmaChannel, true);
  }

  // Restart ADC
  adc_run(true);
}


uint16_t DSP::getSampleSize() {
  return _sampleSize;
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