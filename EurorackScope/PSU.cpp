#include "PSU.h"

#include <hardware/adc.h>
#include <Arduino.h>
#define RP2040_REF 3.3f
#define RP2040_MAX 4095.0f

PSU::PSU() {}

void PSU::begin() {
  analogReadResolution(12);

  _ads.begin(0x48);
  _ads.setGain(GAIN_ONE);  // +/-4.096V
}

void PSU::update() {

  switch (_adsState) {

    case 0:
      _voltages[A][PLUS5] =
        readRP2040(27, (3.06f / 2.0f));  // ADC1
      _adsState++;
      break;

    case 1:
      _voltages[A][PLUS12] =
        readRP2040(26, (46.0f / 12.85));  // ADC0
      _adsState++;
      break;

    case 2:
      if (!_adsBusy) {
        _ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, false);
        _adsBusy = true;
        _lastADSStart = millis();
      } else if (_ads.conversionComplete()) {
        uint16_t raw = _ads.getLastConversionResults();
        _voltages[A][MINUS12] =
          -convertADS(raw, (703.0f / 200.0f), false);
        _adsBusy = false;
        _adsState++;
      }
      break;

    case 3:
      if (!_adsBusy) {
        _ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_3, false);
        _adsBusy = true;
      } else if (_ads.conversionComplete()) {
        uint16_t raw = _ads.getLastConversionResults();
        _voltages[B][PLUS5] =
          convertADS(raw, (238.0f / 100.0f), false);
        _adsBusy = false;
        _adsState++;
      }
      break;

    case 4:
      if (!_adsBusy) {
        _ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_2, false);
        _adsBusy = true;
      } else if (_ads.conversionComplete()) {
        uint16_t raw = _ads.getLastConversionResults();
        _voltages[B][PLUS12] =
          convertADS(raw, (5.77f / 2.0f), false);
        _adsBusy = false;
        _adsState++;
      }
      break;

    case 5:
      if (!_adsBusy) {
        _ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_1, false);
        _adsBusy = true;
      } else if (_ads.conversionComplete()) {
        uint16_t raw = _ads.getLastConversionResults();
        _voltages[B][MINUS12] =
          -convertADS(raw, (707.0f / 200.0f), false);
        _adsBusy = false;
        _adsState = 0;
      }
      break;
  }
}

float PSU::readRP2040(uint8_t pin, float divider) {
 // Temporarily disable DMA interrupts to prevent race conditions
  uint32_t saved = save_and_disable_interrupts();
  
  // Store current ADC state
  bool adc_running = adc_hw->cs & ADC_CS_EN_BITS;
  uint32_t old_div = adc_hw->div;
  
  // Select the input
  adc_select_input(pin - 26);
  
  // Do a single conversion (without changing core configuration)
  adc_hw->cs |= ADC_CS_START_ONCE_BITS;
  while (!(adc_hw->cs & ADC_CS_READY_BITS)) {
    tight_loop_contents();
  }
  
  // Read the result
  uint16_t raw = adc_hw->result;
  
  // Restore previous state if needed
  if (adc_running) {
    // Re-enable continuous mode
    adc_hw->cs |= ADC_CS_EN_BITS;
  }
  
  // Restore interrupts
  restore_interrupts(saved);
  
  float v = (raw / RP2040_MAX) * RP2040_REF;
  return v * divider;
}

float PSU::convertADS(uint16_t raw, float divider, bool negative) {
  float volts = _ads.computeVolts(raw);
  return volts * divider;
}
float PSU::getVoltage(Channel ch, Rail rail) {
  return _voltages[ch][rail];
}