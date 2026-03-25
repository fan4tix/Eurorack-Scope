#include "DSP.h"

DSP* DSP::instance = nullptr;

DSP::DSP(uint8_t pin0, uint8_t pin1)
: _pin0(pin0), _pin1(pin1)
{
    instance = this;
}

void DSP::begin(uint32_t sampleRate, uint16_t sampleSize)
{
    _sampleRate = sampleRate;
    _sampleSize = sampleSize;
    _currentDMABuffer = 0;
    _bufferReady = false;
    
  _channels[0].begin(_pin0, _sampleSize, _sampleRate, _ch0);
  _channels[1].begin(_pin1, _sampleSize, _sampleRate, _ch1);
    setupADC();
    setupDMA();

    adc_run(true);
}

void DSP::setupADC()
{
    adc_init();

    adc_gpio_init(_pin0);
    adc_gpio_init(_pin1);

    uint ch0 = _pin0 - 26;
    uint ch1 = _pin1 - 26;

    adc_set_round_robin((1u << ch0) | (1u << ch1));
    alignADCChannelOrder();

    adc_fifo_setup(
        true,
        true,
        1,
        false,
        false
    );

    // Period = (1 + div) ADC clock cycles @ 48 MHz.
    // Two channels in round-robin → total ADC rate = 2 × _sampleRate.
    float div = 48000000.0f / (2.0f * _sampleRate) - 1.0f;
    adc_set_clkdiv(div);
}

void DSP::alignADCChannelOrder()
{
    uint ch0 = _pin0 - 26;
    uint ch1 = _pin1 - 26;

    // Re-anchor round-robin sequencing so DMA even samples are always channel 0.
    adc_set_round_robin(0);
    adc_select_input(ch0);
    adc_fifo_drain();
    adc_set_round_robin((1u << ch0) | (1u << ch1));
}

void DSP::setupDMA()
{
    _dma = dma_claim_unused_channel(true);

    dma_channel_config cfg = dma_channel_get_default_config(_dma);

    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(
        _dma,
        &cfg,
        _dmaBuffer[0],
        &adc_hw->fifo,
        _sampleSize * 2,
        false
    );

    dma_channel_set_irq0_enabled(_dma, true);

    irq_set_exclusive_handler(DMA_IRQ_0, dmaISR);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_channel_start(_dma);
}

void DSP::dmaISR()
{
    uint32_t status = dma_hw->ints0;

    if(status & (1u << instance->_dma))
    {
        dma_hw->ints0 = 1u << instance->_dma;

        uint8_t next = (instance->_currentDMABuffer + 1) % NUM_DMA_BUFFERS;

        dma_channel_set_write_addr(
            instance->_dma,
            instance->_dmaBuffer[next],
            true
        );

        instance->_currentDMABuffer = next;

        instance->_bufferReady = true;
    }
}

void DSP::update()
{
    
  static uint32_t lastDebugTime = 0;
  if (millis() - lastDebugTime > 1000) {
    lastDebugTime = millis();

    // Check DMA channel status
    dma_channel_hw_t* hw = dma_channel_hw_addr(_dma);
    Serial.print("DMA Busy: ");
    Serial.print(dma_channel_is_busy(_dma));
    Serial.print(" | Transfer count: ");
    Serial.print(hw->transfer_count);
    Serial.print("| FIFO level: ");
    Serial.println(adc_fifo_get_level());
    Serial.print(" | Buffer ready: ");
    Serial.println(_channels[0]._bufferReady);
  }
    if(!_bufferReady) return;

    uint32_t flags = save_and_disable_interrupts();
    _bufferReady = false;

    uint8_t buf = (_currentDMABuffer == 0) ? 1 : 0;
    restore_interrupts(flags);

    uint16_t* src = _dmaBuffer[buf];

    for(uint16_t i=0;i<_sampleSize;i++)
    {
        _ch0[i] = src[i*2];
        _ch1[i] = src[i*2+1];
    }
    _channels[0].processBuffer();
    _channels[1].processBuffer();
}

uint16_t* DSP::getWaveform(uint8_t ch)
{
    if(ch==0) return _ch0;
    else return _ch1;
}

float* DSP::getWaveformVolts(uint8_t ch)
{
    if(ch > 1) return nullptr;
    return _channels[ch].getWaveformVolts();
}

uint16_t DSP::getSampleSize()
{
    return _sampleSize;
}

DSPChannel* DSP::getChannel(uint8_t ch) {
  if (ch > 1) {
    return nullptr;
  }
  return &_channels[ch];
}

uint32_t DSP::getSampleRate()
{
    return _sampleRate;
}
void DSP::changeSampleRate(uint32_t newRate)
{
    if(newRate == _sampleRate)
        return;

    _sampleRate = newRate;
    _channels[0].setSampleRate(_sampleRate);
    _channels[1].setSampleRate(_sampleRate);

    // ---- Disable DMA interrupt ----
    dma_channel_set_irq0_enabled(_dma, false);

    // ---- Stop ADC ----
    adc_run(false);

    // ---- Abort DMA ----
    dma_channel_abort(_dma);

    while(dma_channel_is_busy(_dma))
        tight_loop_contents();

    // ---- Clear FIFO ----
    adc_fifo_drain();

    // ---- Reset round-robin phase so interleaved samples stay ch0/ch1 ----
    alignADCChannelOrder();

    // ---- Clear pending DMA interrupt ----
    dma_hw->ints0 = 1u << _dma;

    // ---- Update ADC clock ----
    float div = 48000000.0f / (2.0f * _sampleRate) - 1.0f;
    adc_set_clkdiv(div);

    // ---- Reset buffer state ----
    _currentDMABuffer = 0;
    _bufferReady = false;

    // ---- Restart DMA ----
    dma_channel_set_write_addr(_dma, _dmaBuffer[0], false);
    dma_channel_set_trans_count(_dma, _sampleSize * 2, false);

    // ---- Re-enable interrupt ----
    dma_channel_set_irq0_enabled(_dma, true);

    // ---- Start DMA ----
    dma_channel_start(_dma);

    // ---- Restart ADC ----
    adc_run(true);
}

void DSP::restartDMA()
{
    _currentDMABuffer = 0;
    _bufferReady = false;

    dma_channel_set_write_addr(
        _dma,
        _dmaBuffer[0],
        false
    );

    dma_channel_set_trans_count(
        _dma,
        _sampleSize * 2,
        false
    );

    dma_channel_start(_dma);
}
