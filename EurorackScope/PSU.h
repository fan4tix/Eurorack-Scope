#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

class PSU {
public:
  enum Channel { A = 0,
                 B };
  enum Rail { PLUS5 = 0,
              PLUS12,
              MINUS12 };

  PSU();

  void begin();
  void update();  // non-blocking round-robin update

  float getVoltage(Channel ch, Rail rail);

private:
  Adafruit_ADS1115 _ads;

  float _voltages[2][3];  // [channel][rail]

  uint8_t _adsState = 0;
  uint32_t _lastADSStart = 0;
  bool _adsBusy = false;

  float readRP2040(uint8_t pin, float divider);
  float convertADS(uint16_t raw, float divider, bool negative);
};