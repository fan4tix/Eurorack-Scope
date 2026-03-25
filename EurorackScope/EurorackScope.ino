
// ADC ADDRESS: 0x48

#include <stdio.h>
#include "pico/stdlib.h"
// For ADC input:
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
// For resistor DAC output:
#include "pico/multicore.h"
#include "hardware/pio.h"

#include <Adafruit_GFX.h>

#include <Adafruit_SH110X.h>
#include "PicoEncoder.h"

#include <SPI.h>
#include <Wire.h>
#include "pinout.h"
#include "App.h"
#include "UI.h"

#include "UIContext.h"
#include "Buttons.h"
#include "DSP.h"
#include "FFTApp.h"
#include "LissajousApp.h"
#include "PSUApp.h"
#include "ScopeApp.h"
#include "TunerApp.h"
#include "VoltmeterApp.h"


// Debug
extern volatile uint32_t interrupt_counter;


// Apps
FFTApp fftApp(&fftMenu);
LissajousApp lissajousApp(&lissaMenu);
PSUApp psuApp(&psuMenu);
ScopeApp scopeApp(&scopeMenu);
TunerApp tunerApp(&tunerMenu);
VoltmeterApp voltmeterApp(&voltmeterMenu);

App* apps[] = {
  &fftApp,
  &lissajousApp,
  &psuApp,
  &scopeApp,
  &tunerApp,
  &voltmeterApp
};

// IO
PicoEncoder encoder;
long lastEncPos = 0;
int8_t encoderDelta = 0;
// Buttons
const uint8_t pins[] = { P_ENC_BTN, P_BACK_BTN };
Buttons buttons(pins, 2, 8);

PSU psu;

DSP dsp(28, 29);  // GPIO28 = ADC2

Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, P_OLED_MOSI, P_OLED_CLK, P_OLED_DC, P_OLED_RESET, P_OLED_CS);

// UI
UIContext ctx = { &display, &dsp, &psu, &buttons, 0 };

UI ui(&ctx,
      apps,
      sizeof(apps) / sizeof(App*));



void setup() {
  Serial.begin(115200);
  Serial.println("Setup start");
  dsp.begin(256, 2048);

  setupWire();
  //setupSPI();
  SPI.begin();
  display.begin(0, true);  // we dont use the i2c address but we will reset!
  display.setRotation(2);
  showBootScreen();
  psu.begin();
  delay(2000);
  pinMode(14, OUTPUT);
  digitalWrite(14, LOW);

  pinMode(P_ENC_BTN, INPUT_PULLUP);
  pinMode(P_BACK_BTN, INPUT_PULLUP);
  encoder.begin(P_ENC_A);
  dsp.update();
  ui.begin();

  Serial.println("Setup OK");
}

bool pinState = false;
void loop() {
  pinState = !pinState;
  digitalWrite(14, pinState ? HIGH : LOW);
  // --------------------------------
  // Input
  // --------------------------------
  // Update dsp
  dsp.update();

  // Update User Input
  buttons.update();
  encoder.update();
  long pos = encoder.position / 256;  // Magic Number
  encoderDelta = lastEncPos - pos;
  lastEncPos = pos;
  //bool encoderPressed = buttons.pressed(P_ENC_BTN);
  //bool backPressed = buttons.pressed(P_BACK_BTN);


  // --------------------------------
  // Logic
  // --------------------------------
  // Update UI
  uint8_t settings_dirty = ui.update(encoderDelta);
  if (settings_dirty) {
    storeSettings();
  }

  static uint32_t t = 0;
  if (millis() - t > 1000) {
    t = millis();
    //  Serial.println("alive");
    //  Serial.print("Interrupts: ");
    //  Serial.println(interrupt_counter);
    //  Serial.print("DMA busy: ");
    //  Serial.println(dma_channel_is_busy(dsp._dmaChannel));
    Serial.print("Waveform[0]: ");
    Serial.println(dsp.getChannel(0)->getWaveformFloat()[0]);
  }
}
void setupWire() {
  Wire.setSDA(P_SDA);
  Wire.setSCL(P_SCL);
  Wire.begin();
}

void setupSPI() {
  SPI.setRX(P_OLED_MISO);  // or setMISO()
  SPI.setCS(P_OLED_CS);
  SPI.setSCK(P_OLED_CLK);
  SPI.setTX(P_OLED_MOSI);  // or setMOSI()
  SPI.begin();
}

void showBootScreen() {

  display.clearDisplay();
  display.drawRect(10, 4, 104, 48, SH110X_WHITE);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(38, 10);
  display.println("Eurorack");
  display.setCursor(46, 22);
  display.println("Scope");
  display.setCursor(16, 34);
  display.println("Initializing...");

  display.display();
}


void storeSettings() {

  /*  settingsEEPROM.data().bpm = bpm;
  settingsEEPROM.data().swing = swing;
  settingsEEPROM.data().ledBrightness = ledBrightness;
  settingsEEPROM.data().ledColor = ledColor;
  settingsEEPROM.markDirty();*/
  //Serial.println("Storing Settings");
}
