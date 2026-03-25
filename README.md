# Eurorack Oscilloscope

A compact, multi-function signal analyser for Eurorack modular synthesisers. Built around an RP2040 microcontroller, it samples two CV/audio inputs simultaneously via DMA-driven ADC and displays the result on a 128×64 OLED. Six apps are accessible from a navigation menu: oscilloscope, FFT spectrum analyser, Lissajous display, chromatic tuner, voltmeter, and PSU monitor.

---

## Features

- **Dual-channel simultaneous sampling** — RP2040 ADC in round-robin DMA mode captures both inputs at sample rates from ~1 Hz up to 200 kHz per channel
- **Six display apps** — each app configures its own optimal sample rate on entry
- **Rotary encoder navigation** — single encoder with push-button + back button controls all menus and in-app settings
- **128×64 SH1106G OLED** — SPI-connected monochrome display
- **Per-rack PSU monitoring** — voltage and current on ±12 V and +5 V rails via INA226 sensors and ADS1115 ADC

---

## Apps

### Oscilloscope
Triggered waveform display with adjustable time base and amplitude scaling.

- **Channels:** CH1, CH2, or CH1+CH2 overlay
- **Time base:** 1 s, 500 ms, 250 ms, 100 ms, 50 ms, 25 ms, 10 ms, 5 ms
- **Amplitude:** ±10 V, ±5 V, ±2 V, ±1 V, ±0.5 V, ±0.2 V, ±0.1 V
- **Trigger:** Rising zero-crossing on the active channel (25 % headroom buffer, 20 mV hysteresis)
- **Readout:** Vpp displayed per trace; dual-channel mode shows both

### FFT Spectrum Analyser
Real-time frequency spectrum using a 2048-point FFT with Hann windowing.

- **Sample rate:** 32768 Hz (~16 fps)
- **Frequency range:** selectable upper limit — 1 kHz, 2 kHz, 4 kHz, 8 kHz, 16 kHz
- **Channel select:** toggle between CH1 and CH2 with encoder press
- **Max range select:** hold encoder press to switch to frequency-range editing mode
- **Readout:** Detected fundamental frequency (Hz) via Harmonic Product Spectrum

### Lissajous Display
XY plot of CH1 (horizontal) vs CH2 (vertical) over the full sample buffer.

- **Time base:** same options as the oscilloscope (controls how much signal history is captured per frame)
- **Amplitude:** same ±V options as the oscilloscope (scales both axes equally)
- **UI overlay:** parameter labels are visible for 2 seconds after any encoder interaction, then the full screen is given to the plot

### Chromatic Tuner
Pitch detection with note name, octave, and cent deviation display.

- **Sample rate:** 4096 Hz
- **Detection:** Harmonic Product Spectrum on a 2048-point FFT
- **Smoothing:** 8-sample moving average on the detected pitch
- **Display:** note name + octave (e.g. `A4`), frequency in Hz, cent deviation bar with a triangle cursor (±50 ¢ range)
- **Channel:** CH1

### Voltmeter
DC voltage, RMS, min and max for both channels simultaneously.

- **Update rate:** ~30 Hz (61440 Hz sample rate / 2048 samples)
- **Display:** large DC readout for CH1 and CH2, with min/max lines below each

### PSU Monitor
Power-supply rail monitoring for two separate PSU buses (A and B).

- **Rails:** +5 V, +12 V, −12 V
- **Modes:** Voltage view and Current view (toggle with encoder)
- **Hardware:** INA226 current-sense ICs (I²C) + ADS1115 ADC for voltage measurement

---

## Hardware

### Components

| Part | Description |
|---|---|
| RP2040 (e.g. Raspberry Pi Pico) | Main microcontroller |
| SH1106G 128×64 OLED | SPI display |
| Rotary encoder with push-button | Navigation |
| Momentary push-button | Back / cancel |
| INA226 × 6 | Current sensing on PSU rails |
| ADS1115 | Voltage measurement on PSU rails |
| Resistor divider network | Scales ±12 V Eurorack signals to RP2040 ADC range |
| Eurorack 3U panel | Front plate (drawings and STL in `Hardware/1.0/`) |

### Signal Input

The two CV/audio inputs are connected to RP2040 GPIO28 (ADC2) and GPIO29 (ADC3).  
A resistor voltage-divider network attenuates and level-shifts the ±12 V Eurorack signal range into the 0–3.3 V ADC input range.

**Calibration constants (baked into firmware — do not change without re-calibrating):**
- `ADC_INPUT_OFFSET_VOLTS = 1.4942857` — mid-rail offset after divider
- `ADC_INPUT_DIVIDER_GAIN = 0.1291666` — divider attenuation factor

### Pin Assignment

| Signal | GPIO |
|---|---|
| Encoder A | GPIO12 |
| Encoder B | GPIO13 |
| Encoder button | GPIO11 |
| Back button | GPIO15 |
| OLED MOSI | GPIO3 |
| OLED CLK | GPIO5 |
| OLED DC | GPIO7 |
| OLED CS | GPIO14 |
| OLED RESET | GPIO8 |
| I²C SDA | GPIO0 |
| I²C SCL | GPIO1 |
| CV Input CH1 | GPIO28 (ADC2) |
| CV Input CH2 | GPIO29 (ADC3) |

### PCB & Front Panel

KiCad board files, 3D STEP exports, and a front panel STL are located in `Hardware/1.0/`.

---

## Software Setup

### 1. Install the RP2040 board package in Arduino IDE

1. Open **Arduino IDE** (2.x recommended).
2. Go to **File → Preferences**.
3. In the *Additional boards manager URLs* field, add:
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
4. Click **OK**, then open **Tools → Board → Boards Manager**.
5. Search for **"Raspberry Pi Pico/RP2040"** (by Earle F. Philhower, III) and click **Install**.
6. After installation, select **Tools → Board → Raspberry Pi RP2040 Boards → Raspberry Pi Pico** (or your specific board variant).

### 2. Install required libraries

Install all of the following via **Tools → Manage Libraries** (or the Library Manager in Arduino IDE 2):

| Library | Author | Purpose |
|---|---|---|
| **Adafruit GFX Library** | Adafruit | Graphics primitives |
| **Adafruit SH110X** | Adafruit | SH1106G OLED driver |
| **Adafruit BusIO** | Adafruit | I²C/SPI abstraction (dependency) |
| **Adafruit ADS1X15** | Adafruit | ADS1115 ADC for PSU voltages |
| **INA226_WE** | Wolfgang Ewald | INA226 current sensing |
| **PicoEncoder** | (search "PicoEncoder") | Interrupt-driven rotary encoder |

The CMSIS-DSP library used for FFT (`arm_math.h`) is bundled with the Earle Philhower RP2040 board package and requires no separate installation.

### 3. Build and flash

1. Clone or download this repository.
2. Open `EurorackScope/EurorackScope.ino` in Arduino IDE.
3. Select the correct board and port under **Tools**.
4. Hold the **BOOTSEL** button on the Pico while connecting USB to enter bootloader mode, then click **Upload**.

---

## Repository Structure

```
EurorackScope/          Arduino sketch and all source files
  EurorackScope.ino     Entry point (setup / loop)
  DSP.cpp / .h          ADC + DMA engine, dual-channel sample acquisition
  DSPChannel.cpp / .h   Per-channel FFT, Hann window, HPS, voltmeter stats
  ScopeApp.cpp / .h     Oscilloscope app
  FFTApp.cpp / .h       FFT spectrum analyser app
  LissajousApp.cpp / .h Lissajous XY display app
  TunerApp.cpp / .h     Chromatic tuner app
  VoltmeterApp.cpp / .h Dual voltmeter app
  PSUApp.cpp / .h       PSU monitor app
  PSU.cpp / .h          INA226 + ADS1115 PSU sensing driver
  UI.cpp / .h           App switcher, menu renderer, encoder handling
  Buttons.cpp / .h      Debounced push-button driver
  App.h                 Base class for all apps (onEnter / onExit / draw)
  Menu.h                Menu data structures and mode constants
  UIContext.h           Context struct passed to every app on each frame
  pinout.h              All GPIO pin definitions
Hardware/
  1.0/
    Board1/             Main PCB (KiCad, STEP, gerbers)
    Board2/             Front panel PCB or daughter board
```

## Add your files

- [ ] [Create](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#create-a-file) or [upload](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#upload-a-file) files
- [ ] [Add files using the command line](https://docs.gitlab.com/topics/git/add_files/#add-files-to-a-git-repository) or push an existing Git repository with the following command:

```
cd existing_repo
git remote add origin http://gitlab.fan4tix.de/embedded/eurorack/eurorack-scope.git
git branch -M main
git push -uf origin main
```

## Integrate with your tools

- [ ] [Set up project integrations](http://gitlab.fan4tix.de/embedded/eurorack/eurorack-scope/-/settings/integrations)

## Collaborate with your team

- [ ] [Invite team members and collaborators](https://docs.gitlab.com/ee/user/project/members/)
- [ ] [Create a new merge request](https://docs.gitlab.com/ee/user/project/merge_requests/creating_merge_requests.html)
- [ ] [Automatically close issues from merge requests](https://docs.gitlab.com/ee/user/project/issues/managing_issues.html#closing-issues-automatically)
- [ ] [Enable merge request approvals](https://docs.gitlab.com/ee/user/project/merge_requests/approvals/)
- [ ] [Set auto-merge](https://docs.gitlab.com/user/project/merge_requests/auto_merge/)

## Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Get started with GitLab CI/CD](https://docs.gitlab.com/ee/ci/quick_start/)
- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing (SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

# Editing this README

When you're ready to make this README your own, just edit this file and use the handy template below (or feel free to structure it however you want - this is just a starting point!). Thanks to [makeareadme.com](https://www.makeareadme.com/) for this template.

## Suggestions for a good README

Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.

## Name
Choose a self-explaining name for your project.

## Description
Let people know what your project can do specifically. Provide context and add a link to any reference visitors might be unfamiliar with. A list of Features or a Background subsection can also be added here. If there are alternatives to your project, this is a good place to list differentiating factors.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.
