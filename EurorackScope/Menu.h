
#ifndef _MENU_H_
#define _MENU_H_
#include <Arduino.h>
#include "UI.h"
#define MODE_FFT 0
#define MODE_LISSA 1
#define MODE_PSU 2
#define MODE_SCOPE 3
#define MODE_TUNER 4
#define MODE_VOLTMETER 5
extern UI ui;
extern int8_t scopeTrigger;
// --------------------------------------------------
enum MenuType {
  MENU_SUBMENU,
  MENU_ACTION,
  MENU_VALUE
};

struct Menu;

struct MenuItem {
  const char* label;
  MenuType type;

  const Menu* submenu;
  void (*action)();
  int8_t* value;

  uint8_t min;
  uint8_t max;
  uint8_t step;
};

struct Menu {
  const char* title;
  const MenuItem* items;
  uint8_t itemCount;
};
// --------------------------------------------------
// callbacks
// --------------------------------------------------
inline void actionFactoryReset() {
 ui.back();
 ui.back();
 ui.back();
 ui.back();
}

inline void actionBack() {
 ui.back();
}

inline void actionScope() {
 ui.changeMode(MODE_SCOPE);
}
inline void actionVoltmeter() {
 ui.changeMode(MODE_VOLTMETER);
}
inline void actionFFT() {
 ui.changeMode(MODE_FFT);
}
inline void actionTuner() {
 ui.changeMode(MODE_TUNER);
}
inline void actionLissa() {
 ui.changeMode(MODE_LISSA);
}

inline void actionPSU() {
 ui.changeMode(MODE_PSU);
}



// --------------------------------------------------
// System
// --------------------------------------------------


const MenuItem factResetConfItems[] = {
  { "No", MENU_ACTION, nullptr, actionBack, nullptr, 0, 0, 0 },
  { "Yes", MENU_ACTION, nullptr, actionFactoryReset, nullptr, 0, 0, 0 }
};

const Menu factResetConfMenu = {
  "Sure?",
  factResetConfItems,
  2

};


const MenuItem systemItems[] = {
  { "Factory Reset", MENU_SUBMENU, &factResetConfMenu, 0, 0, 0, 0, 0 }
};

const Menu systemMenu = {
  "System",
  systemItems,
  1
};




// --------------------------------------------------
// Settings
// --------------------------------------------------
const MenuItem settingsItems[] = {
  { "System", MENU_SUBMENU, &systemMenu, 0, 0, 0, 0, 0 }
};

const Menu settingsMenu = {
  "Settings",
  settingsItems,
  1
  //&mainMenu
};


// --------------------------------------------------
// Mode
// --------------------------------------------------
const MenuItem modeItems[] = {

  { "Scope", MENU_ACTION, nullptr, actionScope, nullptr, 0, 0, 0 },
  { "Voltmeter", MENU_ACTION, nullptr, actionVoltmeter, nullptr, 0, 0, 0 },
  { "FFT", MENU_ACTION, nullptr, actionFFT, nullptr, 0, 0, 0 },
  { "Tuner", MENU_ACTION, nullptr, actionTuner, nullptr, 0, 0, 0 },
  { "Lissajous", MENU_ACTION, nullptr, actionLissa, nullptr, 0, 0, 0 },
  { "PSU Stats", MENU_ACTION, nullptr, actionPSU, nullptr, 0, 0, 0 }


};

const Menu modeMenu = {
  "Select Mode",
  modeItems,
  6
  //NULL
};

// --------------------------------------------------
// Scope
// --------------------------------------------------
const MenuItem scopeItems[] = {

  { "Mode", MENU_SUBMENU, &modeMenu, 0, 0, 0, 0, 0 },
  { "Trigger", MENU_VALUE, nullptr, 0, &scopeTrigger, 0, 2, 1 },
  { "Settings", MENU_SUBMENU, &settingsMenu, 0, 0, 0, 0, 0 }
};


const Menu scopeMenu = {
  "Scope",
  scopeItems,
  2
  //NULL
};

// --------------------------------------------------
// FFT
// --------------------------------------------------
const MenuItem fftItems[] = {

  { "Mode", MENU_SUBMENU, &modeMenu, 0, 0, 0, 0, 0 },
  { "Settings", MENU_SUBMENU, &settingsMenu, 0, 0, 0, 0, 0 }

};


const Menu fftMenu = {
  "FFT",
  fftItems,
  2
  //NULL
};


// --------------------------------------------------
// PSU
// --------------------------------------------------
const MenuItem psuItems[] = {

  { "Mode", MENU_SUBMENU, &modeMenu, 0, 0, 0, 0, 0 },
  { "Settings", MENU_SUBMENU, &settingsMenu, 0, 0, 0, 0, 0 }

};


const Menu psuMenu = {
  "PSU Stats",
  psuItems,
  2
  //NULL
};


// --------------------------------------------------
// Tuner
// --------------------------------------------------
const MenuItem tunerItems[] = {

  { "Mode", MENU_SUBMENU, &modeMenu, 0, 0, 0, 0, 0 },
  { "Settings", MENU_SUBMENU, &settingsMenu, 0, 0, 0, 0, 0 }

};


const Menu tunerMenu = {
  "Tuner",
  tunerItems,
  2
  //NULL
};


// --------------------------------------------------
// Lissajous
// --------------------------------------------------
const MenuItem lissaItems[] = {

  { "Mode", MENU_SUBMENU, &modeMenu, 0, 0, 0, 0, 0 },
  { "Settings", MENU_SUBMENU, &settingsMenu, 0, 0, 0, 0, 0 }

};


const Menu lissaMenu = {
  "Lissajous",
  tunerItems,
  2
  //NULL
};


// --------------------------------------------------
// Voltmeter
// --------------------------------------------------
const MenuItem voltmeterItems[] = {

  { "Mode", MENU_SUBMENU, &modeMenu, 0, 0, 0, 0, 0 },
  { "Settings", MENU_SUBMENU, &settingsMenu, 0, 0, 0, 0, 0 }

};


const Menu voltmeterMenu = {
  "Voltmeter",
  voltmeterItems,
  2
  //NULL
};

#endif