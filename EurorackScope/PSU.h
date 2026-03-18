#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <INA226_WE.h>

// TODO insert correct address
#define INA_A_5V_ADDR   0x40
#define INA_A_12V_ADDR  0x41
#define INA_A_N12V_ADDR 0x42

#define INA_B_5V_ADDR   0x43
#define INA_B_12V_ADDR  0x44
#define INA_B_N12V_ADDR 0x45

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
  float getCurrent(Channel ch, Rail rail);
private:
  // ---- Current sensing ----
  INA226_WE* _ina[2][3];

  float _currents[2][3];  // [channel][rail]

  uint8_t _inaState = 0;
  // ---- Voltages ----
  Adafruit_ADS1115 _ads;

  float _voltages[2][3];  // [channel][rail]

  uint8_t _adsState = 0;
  uint32_t _lastADSStart = 0;
  bool _adsBusy = false;

  float readRP2040(uint8_t pin, float divider);
  float convertADS(uint16_t raw, float divider, bool negative);
};