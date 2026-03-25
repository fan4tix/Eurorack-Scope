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

  // ---- Create INA objects ----
  _ina[A][PLUS5] = new INA226_WE(INA_A_5V_ADDR);
  _ina[A][PLUS12] = new INA226_WE(INA_A_12V_ADDR);
  _ina[A][MINUS12] = new INA226_WE(INA_A_N12V_ADDR);

  _ina[B][PLUS5] = new INA226_WE(INA_B_5V_ADDR);
  _ina[B][PLUS12] = new INA226_WE(INA_B_12V_ADDR);
  _ina[B][MINUS12] = new INA226_WE(INA_B_N12V_ADDR);

  // ---- Init all INA chips ----
  for (int ch = 0; ch < 2; ch++) {
    for (int r = 0; r < 3; r++) {
      _ina[ch][r]->init();

      // 60 mOhm shunt
      //      _ina[ch][r]->setShuntResistorValue(0.06);
      _ina[ch][r]->setCorrectionFactor(1.0);

      _ina[ch][r]->setAverage(INA226_AVERAGE_16);
      _ina[ch][r]->setConversionTime(INA226_CONV_TIME_1100);
    }
  }
}

void PSU::update() {

  switch (_adsState) {

    case 0:
      _voltages[A][PLUS5] =
        readRP2040(27, (10.6f / 2.0f));  // ADC1
      _adsState++;
      break;

    case 1:
      _voltages[A][PLUS12] =
        readRP2040(26, (70.0f / 12.85));  // ADC0
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
          -convertADS(raw, (841.0f / 200.0f), false);
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
          convertADS(raw, (376.0f / 100.0f), false);
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
          convertADS(raw, (42.0f / 2.0f), false);
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
          -convertADS(raw, (883.0f / 200.0f), false);
        _adsBusy = false;
        _adsState = 0;
      }
      break;
  }

  // ---- Current round-robin ----
  switch (_inaState) {

    case 0:
      _currents[A][PLUS5] = _ina[A][PLUS5]->getCurrent_mA() / 1000.0f;
      _inaState++;
      break;

    case 1:
      _currents[A][PLUS12] = _ina[A][PLUS12]->getCurrent_mA() / 1000.0f;
      _inaState++;
      break;

    case 2:
      _currents[A][MINUS12] = _ina[A][MINUS12]->getCurrent_mA() / 1000.0f;
      _inaState++;
      break;

    case 3:
      _currents[B][PLUS5] = _ina[B][PLUS5]->getCurrent_mA() / 1000.0f;
      _inaState++;
      break;

    case 4:
      _currents[B][PLUS12] = _ina[B][PLUS12]->getCurrent_mA() / 1000.0f;
      _inaState++;
      break;

    case 5:
      _currents[B][MINUS12] = _ina[B][MINUS12]->getCurrent_mA() / 1000.0f;
      _inaState = 0;
      break;
  }
}

float PSU::readRP2040(uint8_t pin, float divider) {
  // Temporarily disable DMA interrupts to prevent race conditions
  uint32_t saved = save_and_disable_interrupts();

  // Store current ADC state
  bool adc_running = adc_hw->cs & ADC_CS_EN_BITS;


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

float PSU::getCurrent(Channel ch, Rail rail) {
  return _currents[ch][rail];
}