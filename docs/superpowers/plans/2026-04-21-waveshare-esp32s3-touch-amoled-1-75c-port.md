# Waveshare ESP32-S3-Touch-AMOLED-1.75C Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port the claude-desktop-buddy codebase to the Waveshare ESP32-S3-Touch-AMOLED-1.75C (466×466 round AMOLED) by introducing a multi-board config header system, leaving `main.cpp` and `buddies/` untouched.

**Architecture:** A new `src/boards/` directory holds one header per board containing all hardware constants. `hw/pins.h` becomes a dispatcher that `#include`s the correct header based on a build flag. The `hw/` implementation files use only those constants; the two places where behaviour diverges (TCA9554 vs direct GPIO, AXP IRQ polling) are isolated behind a single `#if BOARD_HAS_TCA9554` in `expander.cpp`.

**Tech Stack:** PlatformIO · Arduino framework · ESP32-S3 · Adafruit_XCA9554 (1.8" only) · Arduino_GFX (SH8601 QSPI AMOLED) · XPowersLib (AXP2101)

---

## File Map

| Action | Path | Responsibility |
|---|---|---|
| Create | `src/boards/board_waveshare_esp32s3_touch_amoled_1_8.h` | All 1.8" hardware constants |
| Create | `src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h` | All 1.75C hardware constants |
| Rewrite | `src/hw/pins.h` | Board dispatcher (3 lines) |
| Modify | `src/hw/display.h` | Swap hardcoded values for BOARD_* macros |
| Modify | `src/hw/expander.h` | Guard TCA9554 extern behind BOARD_HAS_TCA9554 |
| Modify | `src/hw/expander.cpp` | Split implementation on BOARD_HAS_TCA9554 |
| Modify | `platformio.ini` | Base env + two named envs |
| No change | `src/hw/hw.cpp` | API unchanged |
| No change | `src/hw/input.cpp` | hwExpanderAxpIrqLow() behaviour change is in expander.cpp |
| No change | `src/hw/display.cpp` | Already uses constants from headers |
| No change | `src/main.cpp`, `src/buddies/` | Untouched by design |

---

## Task 1: Create 1.8" Board Header

**Files:**
- Create: `src/boards/board_waveshare_esp32s3_touch_amoled_1_8.h`

- [ ] **Step 1: Create the boards directory and 1.8" header**

```cpp
// src/boards/board_waveshare_esp32s3_touch_amoled_1_8.h
#pragma once

// Display physical resolution (SH8601 AMOLED, 2× upscaled from canvas).
// Keep name LCD_W_PHYS / LCD_H_PHYS — display.cpp uses these directly via pins.h.
#define LCD_W_PHYS  368
#define LCD_H_PHYS  448

// Logical canvas (half of physical — upscale done in hwDisplayPush).
// BOARD_ prefix required here: display.h wraps these as constexpr int HW_W = BOARD_HW_W;
// without the prefix the preprocessor would expand "constexpr int HW_W = HW_W" to "constexpr int 184 = 184".
#define BOARD_HW_W        184
#define BOARD_HW_H        224

// Safe draw area: symmetric inset from bezel corners (calibrated with DEBUG_SAFE_BOX)
#define BOARD_SAFE_INSET  8

// QSPI to SH8601
#define PIN_LCD_SDIO0  4
#define PIN_LCD_SDIO1  5
#define PIN_LCD_SDIO2  6
#define PIN_LCD_SDIO3  7
#define PIN_LCD_SCLK   11
#define PIN_LCD_CS     12
// LCD_RESET and TP_RESET are driven via TCA9554 expander (see EXIO_* below)

// I2C bus (shared: TCA9554, AXP2101, FT3168, ES8311)
#define PIN_I2C_SDA   15
#define PIN_I2C_SCL   14

// FT3168 touch interrupt — direct to ESP32
#define PIN_TP_INT    21

// I2S to ES8311 codec
#define PIN_I2S_MCLK  16
#define PIN_I2S_BCLK  9
#define PIN_I2S_WS    45
#define PIN_I2S_DI    10
#define PIN_I2S_DO    8
#define PIN_PA_CTRL   46

// Buttons
#define PIN_KEY1      0   // GPIO0 BOOT key, active-low

// TCA9554 I2C GPIO expander pin assignments
#define BOARD_HAS_TCA9554  1
#define EXIO_LCD_RESET     0
#define EXIO_TP_RESET      1
#define EXIO_DSI_PWR_EN    2
#define EXIO_AXP_IRQ       5
```

- [ ] **Step 2: Verify the file content matches the current `src/hw/pins.h` and `src/hw/display.h` exactly**

Cross-check: `PIN_LCD_SCLK=11`, `PIN_TP_INT=21`, `BOARD_HW_W=184`, `BOARD_HW_H=224`, `BOARD_SAFE_INSET=8`.

- [ ] **Step 3: Commit**

```bash
git add src/boards/board_waveshare_esp32s3_touch_amoled_1_8.h
git commit -m "boards: add 1.8 board header (extracted from hw/pins.h + display.h)"
```

---

## Task 2: Create 1.75C Board Header

**Files:**
- Create: `src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h`

- [ ] **Step 1: Create the 1.75C header**

```cpp
// src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h
#pragma once

// Display: 466×466 round AMOLED (SH8601, assumed same driver as 1.8")
// Physical 466×466 is circular; corners are outside the visible area.
// Keep name LCD_W_PHYS / LCD_H_PHYS — display.cpp uses these directly via pins.h.
#define LCD_W_PHYS  466
#define LCD_H_PHYS  466

// Logical canvas (half of physical — upscale done in hwDisplayPush).
// BOARD_ prefix required: see note in 1.8" header about constexpr self-reference.
#define BOARD_HW_W        233
#define BOARD_HW_H        233

// Safe area: rectangle inscribed in the display circle.
// Calculated: radius=116.5, inscribed square side = 116.5×√2 ≈ 164 px.
// inset = (233-164)/2 ≈ 35. Calibrate with DEBUG_SAFE_BOX on device.
#define BOARD_SAFE_INSET  35

// QSPI to SH8601
#define PIN_LCD_SDIO0  4
#define PIN_LCD_SDIO1  5
#define PIN_LCD_SDIO2  6
#define PIN_LCD_SDIO3  7
#define PIN_LCD_SCLK   38
#define PIN_LCD_CS     12

// LCD and TP reset are direct GPIOs (no TCA9554 on this board)
#define PIN_LCD_RESET  1
#define PIN_TP_RESET   2

// LCD tearing-effect output (not used yet; reserved)
#define PIN_LCD_TE     13

// I2C bus (shared: AXP2101, FT3168, ES8311, QMI8658C)
#define PIN_I2C_SDA   15
#define PIN_I2C_SCL   14

// FT3168 touch interrupt — direct to ESP32
#define PIN_TP_INT    11

// I2S to ES8311 codec (identical to 1.8")
#define PIN_I2S_MCLK  16
#define PIN_I2S_BCLK  9
#define PIN_I2S_WS    45
#define PIN_I2S_DI    10
#define PIN_I2S_DO    8
#define PIN_PA_CTRL   46

// Buttons
#define PIN_KEY1      0   // GPIO0 BOOT key, active-low

// No TCA9554 expander on this board
#define BOARD_HAS_TCA9554  0
// AXP_IRQ is not wired to any ESP32 GPIO on the 1.75C; AXP PEK events
// are detected by polling AXP registers via I2C every frame instead.
```

- [ ] **Step 2: Commit**

```bash
git add src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h
git commit -m "boards: add 1.75C board header"
```

---

## Task 3: Rewrite pins.h as Board Dispatcher

**Files:**
- Modify: `src/hw/pins.h`

- [ ] **Step 1: Replace the entire file**

```cpp
// src/hw/pins.h
#pragma once

#if defined(BOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_8)
  #include "../boards/board_waveshare_esp32s3_touch_amoled_1_8.h"
#elif defined(BOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_75C)
  #include "../boards/board_waveshare_esp32s3_touch_amoled_1_75c.h"
#else
  #error "No board defined. Add -DBOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_8 or _1_75C to build_flags in platformio.ini."
#endif
```

- [ ] **Step 2: Commit**

```bash
git add src/hw/pins.h
git commit -m "hw: pins.h becomes board-config dispatcher"
```

---

## Task 4: Update display.h to Use BOARD_* Macros

**Files:**
- Modify: `src/hw/display.h`

- [ ] **Step 1: Replace the hardcoded canvas constants**

Current file has `constexpr int HW_W = 184;` etc. Replace with:

```cpp
// src/hw/display.h
#pragma once
#include <Arduino_GFX_Library.h>
#include "hw/pins.h"   // provides BOARD_HW_W, BOARD_HW_H, BOARD_SAFE_INSET

constexpr int HW_W       = BOARD_HW_W;
constexpr int HW_H       = BOARD_HW_H;

// Logical-canvas safe-draw region for the rounded/circular AMOLED bezel.
constexpr int SAFE_INSET = BOARD_SAFE_INSET;
constexpr int SAFE_L     = SAFE_INSET;
constexpr int SAFE_T     = SAFE_INSET;
constexpr int SAFE_R     = HW_W - SAFE_INSET;
constexpr int SAFE_B     = HW_H - SAFE_INSET;
constexpr int SAFE_W     = HW_W - 2 * SAFE_INSET;
constexpr int SAFE_H     = HW_H - 2 * SAFE_INSET;

bool hwDisplayInit();
void hwDisplayPush();
void hwDisplayBrightness(uint8_t lvl_0_4);
void hwDisplaySleep(bool off);
Arduino_Canvas* hwCanvas();
```

Note: `display.cpp` uses `LCD_W_PHYS` and `LCD_H_PHYS` which come from the board header via `pins.h` (already included in `display.cpp`). No change needed in `display.cpp`.

- [ ] **Step 2: Commit**

```bash
git add src/hw/display.h
git commit -m "hw: display.h reads canvas size from board config"
```

---

## Task 5: Split expander.h and expander.cpp on BOARD_HAS_TCA9554

**Files:**
- Modify: `src/hw/expander.h`
- Modify: `src/hw/expander.cpp`

- [ ] **Step 1: Update expander.h to guard TCA9554-specific declarations**

```cpp
// src/hw/expander.h
#pragma once
#include "hw/pins.h"

#if BOARD_HAS_TCA9554
#include <Adafruit_XCA9554.h>
// Shared TCA9554 instance — used by expander.cpp only;
// other hw/ files go through hwExpanderAxpIrqLow() etc.
extern Adafruit_XCA9554 g_expander;
#endif

bool hwExpanderInit();          // false on I2C failure (TCA9554 path) or never fails (direct GPIO path)
void hwExpanderResetSequence(); // pull LCD_RESET + TP_RESET low → 20ms → high
bool hwExpanderAxpIrqLow();     // true while AXP IRQ is asserted; always true on 1.75C (poll via I2C)
```

- [ ] **Step 2: Rewrite expander.cpp with both paths**

```cpp
// src/hw/expander.cpp
#include "hw/expander.h"
#include "hw/pins.h"
#include <Wire.h>
#include <Arduino.h>

#if BOARD_HAS_TCA9554

Adafruit_XCA9554 g_expander;

bool hwExpanderInit() {
  if (!g_expander.begin(0x20)) return false;
  g_expander.pinMode(EXIO_LCD_RESET,  OUTPUT);
  g_expander.pinMode(EXIO_TP_RESET,   OUTPUT);
  g_expander.pinMode(EXIO_DSI_PWR_EN, OUTPUT);
  g_expander.pinMode(EXIO_AXP_IRQ,    INPUT);
  return true;
}

void hwExpanderResetSequence() {
  g_expander.digitalWrite(EXIO_LCD_RESET,  LOW);
  g_expander.digitalWrite(EXIO_TP_RESET,   LOW);
  g_expander.digitalWrite(EXIO_DSI_PWR_EN, LOW);
  delay(20);
  g_expander.digitalWrite(EXIO_LCD_RESET,  HIGH);
  g_expander.digitalWrite(EXIO_TP_RESET,   HIGH);
  g_expander.digitalWrite(EXIO_DSI_PWR_EN, HIGH);
  delay(20);
}

bool hwExpanderAxpIrqLow() {
  return g_expander.digitalRead(EXIO_AXP_IRQ) == 0;
}

#else  // No TCA9554: LCD_RESET and TP_RESET are direct GPIOs

bool hwExpanderInit() {
  pinMode(PIN_LCD_RESET, OUTPUT);
  pinMode(PIN_TP_RESET,  OUTPUT);
  return true;
}

void hwExpanderResetSequence() {
  digitalWrite(PIN_LCD_RESET, LOW);
  digitalWrite(PIN_TP_RESET,  LOW);
  delay(20);
  digitalWrite(PIN_LCD_RESET, HIGH);
  digitalWrite(PIN_TP_RESET,  HIGH);
  delay(20);
}

// AXP_IRQ is not wired to any GPIO on the 1.75C.
// Returning true causes scanAxp() in input.cpp to poll AXP PEK registers
// via I2C every frame, which is fast enough and avoids a missed press.
bool hwExpanderAxpIrqLow() {
  return true;
}

#endif
```

- [ ] **Step 3: Commit**

```bash
git add src/hw/expander.h src/hw/expander.cpp
git commit -m "hw: expander splits on BOARD_HAS_TCA9554; 1.75C uses direct GPIO + I2C poll"
```

---

## Task 6: Update platformio.ini

**Files:**
- Modify: `platformio.ini`

- [ ] **Step 1: Restructure into base + two named envs**

Replace the entire file:

```ini
[env]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/53.03.13/platform-espressif32.zip
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
upload_speed = 921600

board_build.mcu = esp32s3
board_build.f_cpu = 240000000L
board_build.flash_mode = qio
board_build.psram_type = opi
board_upload.flash_size = 8MB
board_build.partitions = no_ota_8mb.csv
board_build.filesystem = littlefs
board_build.arduino.memory_type = qio_opi

build_src_filter = +<*> +<buddies/> +<hw/> -<main.cpp.m5stick.bak> -<xfer.h>

lib_deps =
    moononournation/GFX Library for Arduino
    lewisxhe/XPowersLib
    lewisxhe/SensorLib
    adafruit/Adafruit BusIO
    bitbank2/AnimatedGIF @ ^2.1.1
    bblanchon/ArduinoJson @ ^7.0.0
    h2zero/NimBLE-Arduino @ ^1.4
    olikraus/U8g2 @ ^2.35

[env:waveshare-esp32s3-touch-amoled-1-8]
build_flags =
    -DCORE_DEBUG_LEVEL=0
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DXPOWERS_CHIP_AXP2101
    -DU8G2_USE_LARGE_FONTS
    -DU8G2_FONT_SUPPORT
    -DBOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_8

[env:waveshare-esp32s3-touch-amoled-1-75c]
build_flags =
    -DCORE_DEBUG_LEVEL=0
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DXPOWERS_CHIP_AXP2101
    -DU8G2_USE_LARGE_FONTS
    -DU8G2_FONT_SUPPORT
    -DBOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_75C
```

- [ ] **Step 2: Commit**

```bash
git add platformio.ini
git commit -m "pio: base env + waveshare-1-8 + waveshare-1-75c environments"
```

---

## Task 7: Verify Both Environments Compile

**Files:** No new changes — fix any errors found.

- [ ] **Step 1: Compile the 1.8" environment**

```bash
pio run -e waveshare-esp32s3-touch-amoled-1-8
```

Expected: `SUCCESS` with no errors. Common failure modes:
- `BOARD_HW_W undeclared` → board header not included; check pins.h dispatcher and that build_flags matches the `#if defined(...)` guard exactly
- `Adafruit_XCA9554.h: No such file` → the `#if BOARD_HAS_TCA9554` guard in expander.h is not picking up the flag; check that `BOARD_HAS_TCA9554` is `1` in the 1.8" board header

- [ ] **Step 2: Compile the 1.75C environment**

```bash
pio run -e waveshare-esp32s3-touch-amoled-1-75c
```

Expected: `SUCCESS` with no errors. Common failure modes:
- `PIN_LCD_RESET undeclared` in expander.cpp → 1.75C board header missing that define
- `EXIO_LCD_RESET undeclared` → the `#else` branch accidentally using an EXIO constant; check the `#if`/`#else` split in expander.cpp

- [ ] **Step 3: If any error, fix and re-run before committing**

Any fixes made in steps 1–2 should be committed as part of those steps. If both envs compiled first try, no extra commit is needed — proceed to Task 8.

---

## Task 8: On-Device Smoke Test (1.75C)

**Files:** Possibly modify `BOARD_SAFE_INSET` in `src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h`

- [ ] **Step 1: Flash the 1.75C board**

Connect the 1.75C, then:
```bash
pio run -e waveshare-esp32s3-touch-amoled-1-75c --target upload
pio device monitor -e waveshare-esp32s3-touch-amoled-1-75c
```

Expected serial output:
```
=== claude-buddy waveshare boot ===
hwInit OK
```

If `hwInit FAIL: expander` → check `PIN_LCD_RESET` and `PIN_TP_RESET` in the 1.75C board header; they must be valid output-capable GPIOs.

If `hwInit FAIL: display` → the SH8601 init failed. Verify `PIN_LCD_SCLK=38`, `PIN_LCD_CS=12`, QSPI pins 4–7 in the board header match the schematic.

- [ ] **Step 2: Verify display shows buddy character**

The buddy character should appear on the round display. Corners will be clipped by the circle — that is expected. The character should be visible and not cut off at the center.

- [ ] **Step 3: Calibrate SAFE_INSET with DEBUG_SAFE_BOX**

Add `-DDEBUG_SAFE_BOX` temporarily to the 1.75C `build_flags` in `platformio.ini`, flash, observe the green rectangle border. Adjust `BOARD_SAFE_INSET` in the 1.75C board header until the green border sits just inside the visible circle edge, then remove `-DDEBUG_SAFE_BOX`.

- [ ] **Step 4: Verify AXP power button works**

Press and release the power button quickly. The buddy's B-button action should trigger (short press event). If it does not respond, check that `hwAxpPekeyShortPress()` in `power.cpp` is being called (it is, because `hwExpanderAxpIrqLow()` always returns `true` on 1.75C).

- [ ] **Step 5: Commit final SAFE_INSET value**

```bash
git add src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h
git commit -m "boards: calibrate 1.75C SAFE_INSET to <value> from on-device test"
```
