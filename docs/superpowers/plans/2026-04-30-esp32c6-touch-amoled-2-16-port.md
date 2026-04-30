# Waveshare ESP32-C6-Touch-AMOLED-2.16 Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a third PlatformIO env to this project that runs the existing buddy on the Waveshare ESP32-C6-Touch-AMOLED-2.16 (480×480 SH8601 with rounded corners; AXP2101 + ES8311 + PCF85063 + QMI8658 + CST9217). All hardware variation expressed via capability flags in `src/boards/`; zero behavior change on 1.8 / 1.75C.

**Architecture:** Three layers of change. (1) New flag values added to existing 1.8 / 1.75C headers preserve their behavior. (2) `hw/*.cpp` reads the new flags to support C6-only paths (display offset, PA gating, LCD reset via PMU, AXP-PWRON-4s config, KEY1 active-HIGH, secondary GPIO key, third button as menu shortcut). (3) The 2.16 board header sets the new flag values; per-env `build_src_filter` selects `ble_bridge_nimble.cpp` (vendored from the C6-LCD sibling repo) instead of the existing Bluedroid `ble_bridge.cpp` because C6 only supports NimBLE.

**Tech Stack:** PlatformIO (pioarduino 53.x) · Arduino 3.x · ESP32-C6 · `Arduino_GFX` (SH8601 QSPI) · `XPowersLib` (AXP2101) · `SensorLib` (QMI8658 + CST92xx) · vendored `lib/ES8311/` · `NimBLE-Arduino @ ^2.0` (2.16 only) · stock Bluedroid BLE (1.8 / 1.75C, unchanged)

**Source references:**
- Spec: `docs/superpowers/specs/2026-04-30-esp32c6-touch-amoled-2-16-port-design.md`
- Schematic: `~/Desktop/ESP32-C6-Touch-AMOLED-2.16-Schematic.pdf`
- Vendor demo: `~/Downloads/ESP32-C6-Touch-AMOLED-2.16/02_Example/Arduino-v3.3.3/`
- XiaoZhi board def: `~/Downloads/ESP32-C6-Touch-AMOLED-2.16/02_Example/XiaoZhi-v2.2.5/main/boards/waveshare/esp32-c6-touch-amoled-2.16/`
- NimBLE ble_bridge to copy: `~/Documents/claude-desktop-buddy-lcd/src/ble_bridge.cpp`

**Note on verification:** This is a hardware port. There are no unit tests. Tasks that change shared `.cpp` files end with `pio run -e <s3-env>` build verification (and a flash + smoke test on at least one S3 board for any task that could change S3 runtime behavior). Tasks adding 2.16-only files end with `pio run -e waveshare-esp32c6-touch-amoled-2-16` build verification.

---

## File Map

| Action | Path | Responsibility |
|---|---|---|
| Modify | `src/boards/board_waveshare_esp32s3_touch_amoled_1_8.h` | Add 8 new flag defaults preserving 1.8 behavior |
| Modify | `src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h` | Add 8 new flag defaults preserving 1.75C behavior |
| Create | `src/boards/board_waveshare_esp32c6_touch_amoled_2_16.h` | All 2.16 hardware constants and capability flags |
| Modify | `src/hw/pins.h` | Add `#elif` branch for the new board macro |
| Modify | `src/hw/display.cpp` | Read `BOARD_DISPLAY_OFFSET_X/Y` in non-letterbox push |
| Modify | `src/hw/input.cpp` | Active-HIGH KEY1, optional KEY2, optional BOOT-key menu shortcut, scanAxp gated by `!BOARD_HAS_KEY2`, touch remap subtracts offset |
| Modify | `src/hw/audio.cpp` | Gate PA enable on `BOARD_HAS_PA_CTRL` |
| Modify | `src/hw/power.cpp` | Add `enableALDO3()` + gated 4s-PWRON-off config |
| Modify | `src/hw/hw.cpp` | Gate panel-reset path on `BOARD_LCD_RST_VIA_PMU` |
| Create | `src/ble_bridge_nimble.cpp` | NimBLE port of NUS bridge (verbatim copy from `claude-desktop-buddy-lcd`) |
| Modify | `platformio.ini` | Add `[env:waveshare-esp32c6-touch-amoled-2-16]` block; `build_src_filter` per env to select Bluedroid vs NimBLE source; remove dead `NimBLE-Arduino` from `[env]` lib_deps; add `^2.0` only in 2.16 env |
| Modify | `README.md` | Add 2.16 to the supported-boards table; pin map; controls matrix |

`src/main.cpp`, `src/buddy.cpp`, `src/buddies/`, `src/data.h`, `src/stats.h`, `src/character.cpp`, `src/xfer.h`, `src/hw/expander.cpp`, `src/hw/rtc.cpp`, `src/hw/imu.cpp` are **not modified** — capability flags already cover the diffs they need.

---

## Phase 1 — Flag Plumbing (no S3 behavior change)

The phase ordering is deliberate: every shared `.cpp` change goes in *first*, with default flag values that match the current 1.8 / 1.75C behavior, so each commit is regression-tested in isolation. Only after the shared changes are stable does the 2.16-specific code land.

### Task 1: Add new flag defaults to 1.8 header

**Files:**
- Modify: `src/boards/board_waveshare_esp32s3_touch_amoled_1_8.h`

- [ ] **Step 1: Open the 1.8 header and append the new flag block**

After the existing `BOARD_HAS_PCF85063  1` line, add:

```cpp
// New flags introduced by 2.16 port. Defaults preserve current 1.8 behavior.
#define BOARD_HAS_PSRAM            1
#define BOARD_DISPLAY_OFFSET_X     0
#define BOARD_DISPLAY_OFFSET_Y     0
#define BOARD_HAS_PA_CTRL          1
#define BOARD_HAS_AXP2101          1
#define BOARD_LCD_RST_VIA_PMU      0
#define BOARD_AXP_PWRON_4S_OFF     0
#define BOARD_BTN_THIRD            0
#define BOARD_KEY1_ACTIVE_HIGH     0
#define BOARD_HAS_KEY2             0
```

- [ ] **Step 2: Build the 1.8 env to verify the header still compiles**

Run: `pio run -e waveshare-esp32s3-touch-amoled-1-8`
Expected: `[SUCCESS]`. No new flags are read yet by any `.cpp`, so this is a tautology check.

- [ ] **Step 3: Commit**

```bash
git add src/boards/board_waveshare_esp32s3_touch_amoled_1_8.h
git commit -m "boards/1.8: declare flags for upcoming 2.16 port (no behavior change)"
```

---

### Task 2: Add new flag defaults to 1.75C header

**Files:**
- Modify: `src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h`

- [ ] **Step 1: Append the same new flag block at end of file**

```cpp
// New flags introduced by 2.16 port. Defaults preserve current 1.75C behavior.
#define BOARD_HAS_PSRAM            1
#define BOARD_DISPLAY_OFFSET_X     0
#define BOARD_DISPLAY_OFFSET_Y     0
#define BOARD_HAS_PA_CTRL          1
#define BOARD_HAS_AXP2101          1
#define BOARD_LCD_RST_VIA_PMU      0
#define BOARD_AXP_PWRON_4S_OFF     0
#define BOARD_BTN_THIRD            0
#define BOARD_KEY1_ACTIVE_HIGH     0
#define BOARD_HAS_KEY2             0
```

- [ ] **Step 2: Build the 1.75C env**

Run: `pio run -e waveshare-esp32s3-touch-amoled-1-75c`
Expected: `[SUCCESS]`.

- [ ] **Step 3: Commit**

```bash
git add src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h
git commit -m "boards/1.75c: declare flags for upcoming 2.16 port (no behavior change)"
```

---

### Task 3: `display.cpp` — read OFFSET_X/Y in non-letterbox push

**Files:**
- Modify: `src/hw/display.cpp:135-147` (the `#else` branch — "2× integer upscale, per-row")

- [ ] **Step 1: Update the non-letterbox blit to add the offset**

Replace the existing non-letterbox loop (lines 137–146) with:

```cpp
  // 2× integer upscale, per-row, with optional centring offset.
  // OFFSET_X/Y are 0 on full-fill boards (1.8); positive on boards
  // where the canvas is smaller than the panel (2.16 → 56/16).
  for (int y = 0; y < HW_H; y++) {
    uint16_t* row = src + y * HW_W;
    for (int x = 0; x < HW_W; x++) {
      uint16_t c = row[x];
      s_lineBuf[x*2]     = c;
      s_lineBuf[x*2 + 1] = c;
    }
    int dy = y * 2 + BOARD_DISPLAY_OFFSET_Y;
    s_gfx->draw16bitRGBBitmap(BOARD_DISPLAY_OFFSET_X, dy,     s_lineBuf, HW_W*2, 1);
    s_gfx->draw16bitRGBBitmap(BOARD_DISPLAY_OFFSET_X, dy + 1, s_lineBuf, HW_W*2, 1);
  }
```

- [ ] **Step 2: Build both S3 envs**

Run:
```bash
pio run -e waveshare-esp32s3-touch-amoled-1-8
pio run -e waveshare-esp32s3-touch-amoled-1-75c
```
Expected: both `[SUCCESS]`.

- [ ] **Step 3: Flash 1.8, smoke-check that the canvas still fills the panel**

Run: `pio run -e waveshare-esp32s3-touch-amoled-1-8 -t upload --upload-port /dev/cu.usbmodem*`
Then visually verify: clock + buddy on screen, no black border anywhere. The canvas should still cover the full 368×448 panel.

- [ ] **Step 4: Commit**

```bash
git add src/hw/display.cpp
git commit -m "display: add OFFSET_X/Y to non-letterbox push path"
```

---

### Task 4: `audio.cpp` — gate PA enable on `BOARD_HAS_PA_CTRL`

**Files:**
- Modify: `src/hw/audio.cpp:94-96`

- [ ] **Step 1: Wrap the PA enable lines**

Change lines 94-96 from:
```cpp
bool hwAudioInit() {
  pinMode(PIN_PA_CTRL, OUTPUT);
  digitalWrite(PIN_PA_CTRL, HIGH);
```

to:
```cpp
bool hwAudioInit() {
#if BOARD_HAS_PA_CTRL
  pinMode(PIN_PA_CTRL, OUTPUT);
  digitalWrite(PIN_PA_CTRL, HIGH);
#endif
```

- [ ] **Step 2: Add the `pins.h` include if not already there**

Check the top of `src/hw/audio.cpp`. The file already includes `"hw/pins.h"` on line 2. No change needed; just verify.

- [ ] **Step 3: Build both S3 envs**

Run:
```bash
pio run -e waveshare-esp32s3-touch-amoled-1-8
pio run -e waveshare-esp32s3-touch-amoled-1-75c
```
Expected: both `[SUCCESS]`.

- [ ] **Step 4: Flash 1.8, audio smoke check**

Trigger any in-app sound (open menu → setting that beeps, or first-pair confirmation chime). Verify it still plays.

- [ ] **Step 5: Commit**

```bash
git add src/hw/audio.cpp
git commit -m "audio: gate PA enable on BOARD_HAS_PA_CTRL"
```

---

### Task 5: `power.cpp` — `enableALDO3` + gated 4s-PWRON-off

**Files:**
- Modify: `src/hw/power.cpp:8-26` (the `hwPowerInit()` function)

- [ ] **Step 1: Add the new register writes inside `hwPowerInit()`**

Change `hwPowerInit()` to add `enableALDO3` + gated PWRON config right after `enableTemperatureMeasure()`:

```cpp
bool hwPowerInit() {
  if (!s_pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, PIN_I2C_SDA, PIN_I2C_SCL)) {
    Serial.println("hwPower: AXP2101 begin failed");
    return false;
  }
  s_pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
  s_pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
  s_pmu.enableBattDetection();
  s_pmu.enableVbusVoltageMeasure();
  s_pmu.enableBattVoltageMeasure();
  s_pmu.enableTemperatureMeasure();

  // ALDO3 → display rail on all three boards. enableALDO3() is idempotent.
  s_pmu.enableALDO3();

#if BOARD_AXP_PWRON_4S_OFF
  // 2.16: configure AXP to power off on 4 s PWRON-hold (the PWR key is
  // the only software-configurable shutdown path on this board).
  // 0x22 reg: PWRON > OFFLEVEL as POWEROFF source. 0x27 reg: 4s timing.
  s_pmu.writeRegister(0x22, 0b110);
  s_pmu.writeRegister(0x27, 0x10);
#endif

  s_pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  s_pmu.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ);
  s_pmu.clearIrqStatus();
  return true;
}
```

- [ ] **Step 2: Verify XPowersLib exposes `writeRegister`**

Quick check:
```bash
grep -rn "writeRegister" .pio/libdeps/*/XPowersLib/src 2>/dev/null | head
```
Expected: at least one match in the public `XPowersCommon.tpp` header. If the symbol is named differently (`writeReg`, `writeByte`, etc.), use that name instead — the spec is design-level; the exact API is library-specific.

- [ ] **Step 3: Build both S3 envs**

Run:
```bash
pio run -e waveshare-esp32s3-touch-amoled-1-8
pio run -e waveshare-esp32s3-touch-amoled-1-75c
```
Expected: both `[SUCCESS]`.

- [ ] **Step 4: Flash 1.8, smoke check**

Boot the board. Verify it still boots cleanly, the panel lights up (proves `enableALDO3()` is idempotent and didn't break a sequencing assumption), AXP IRQ + PEK short/long press still work.

- [ ] **Step 5: Commit**

```bash
git add src/hw/power.cpp
git commit -m "power: idempotent enableALDO3 + gated PWRON 4s-off config"
```

---

### Task 6: `hw.cpp` — gate panel reset on `BOARD_LCD_RST_VIA_PMU`

**Files:**
- Modify: `src/hw/hw.cpp:18-20`

- [ ] **Step 1: Pull the PMU init forward and gate the reset path**

Replace lines 18-20 of `hwInit()` with:

```cpp
  if (!hwExpanderInit())  die("expander");
#if BOARD_LCD_RST_VIA_PMU
  // 2.16 has no LCD_RST GPIO; the panel is reset by power-cycling
  // AXP ALDO3. PMU must be initialised before the display.
  if (!hwPowerInit())     die("power");
  // ALDO3 power-cycle resets the panel (50 ms low between two highs).
  // s_pmu.enableALDO3() in powerInit left it enabled; toggle it here.
  extern XPowersPMU* hwPmuRef();  // see Step 2
  hwPmuRef()->disableALDO3();
  delay(50);
  hwPmuRef()->enableALDO3();
  delay(50);
#else
  hwExpanderResetSequence();
#endif
  if (!hwDisplayInit())   die("display");
#if !BOARD_LCD_RST_VIA_PMU
  if (!hwPowerInit())     die("power");
#endif
  if (!hwInputInit())     die("input");
  if (!hwImuInit())       die("imu");
  if (!hwRtcInit())       die("rtc");
  if (!hwAudioInit())     die("audio");
```

- [ ] **Step 2: Expose the PMU pointer from `power.cpp`**

In `src/hw/power.h`, add at the bottom:

```cpp
class XPowersPMU;
XPowersPMU* hwPmuRef();   // raw access for boards that need direct register / rail control
```

In `src/hw/power.cpp`, add at the bottom of the file:

```cpp
XPowersPMU* hwPmuRef() { return &s_pmu; }
```

- [ ] **Step 3: Build both S3 envs**

Run:
```bash
pio run -e waveshare-esp32s3-touch-amoled-1-8
pio run -e waveshare-esp32s3-touch-amoled-1-75c
```
Expected: both `[SUCCESS]`. The 1.8 / 1.75C path runs `hwExpanderResetSequence()` exactly as before because `BOARD_LCD_RST_VIA_PMU=0`.

- [ ] **Step 4: Flash 1.8, smoke check**

Boot the board. Verify the panel comes up normally (the reset sequence is byte-identical to the previous build).

- [ ] **Step 5: Commit**

```bash
git add src/hw/hw.cpp src/hw/power.cpp src/hw/power.h
git commit -m "hw: gate panel reset on BOARD_LCD_RST_VIA_PMU; expose hwPmuRef()"
```

---

### Task 7: `input.cpp` — KEY1 active level + optional KEY2 + optional BOOT-key shortcut + touch offset

**Files:**
- Modify: `src/hw/input.cpp:63-70` (`scanKey1`), and add new `scanKey2` / `scanBootKey` / change `scanAxp` gating, and `scanTouch` non-letterbox branch (lines 119-122)

- [ ] **Step 1: Make `scanKey1()` honor `BOARD_KEY1_ACTIVE_HIGH`**

Replace the existing `scanKey1()` (lines 63-70) with:

```cpp
static void scanKey1() {
  uint32_t now = millis();
#if BOARD_KEY1_ACTIVE_HIGH
  bool pressed = digitalRead(PIN_KEY1) == HIGH;
#else
  bool pressed = digitalRead(PIN_KEY1) == LOW;
#endif
  s_a.wasPressed  = pressed && !s_a.isPressed;
  s_a.wasReleased = !pressed && s_a.isPressed;
  if (s_a.wasPressed) s_a.pressedAt = now;
  s_a.isPressed = pressed;
}
```

- [ ] **Step 2: Add `scanKey2()` gated by `BOARD_HAS_KEY2`**

Add after `scanKey1()`:

```cpp
#if BOARD_HAS_KEY2
static void scanKey2() {
  uint32_t now = millis();
  bool pressed = digitalRead(PIN_KEY2) == LOW;
  s_b.wasPressed  = pressed && !s_b.isPressed;
  s_b.wasReleased = !pressed && s_b.isPressed;
  if (s_b.wasPressed) s_b.pressedAt = now;
  s_b.isPressed = pressed;
}
#endif
```

- [ ] **Step 3: Add `scanBootKey()` gated by `BOARD_BTN_THIRD`**

Add right below `scanKey2()`:

```cpp
#if BOARD_BTN_THIRD
// BOOT key (GPIO9 on 2.16) acts as a menu shortcut: a short tap synthesises
// BTN_A_LONG_PRESS, which main.cpp's existing handler treats as "open menu".
// main.cpp itself is unchanged.
static uint32_t s_bootPressedAt = 0;
static void scanBootKey() {
  bool pressed = digitalRead(PIN_KEY_BOOT) == LOW;
  if (pressed && !s_bootPressedAt) {
    s_bootPressedAt = millis();
  } else if (!pressed && s_bootPressedAt) {
    uint32_t held = millis() - s_bootPressedAt;
    s_bootPressedAt = 0;
    if (held > 30 && held < 1000) {
      // Synthesise a long-press of A so the menu opens.
      s_a.wasPressed  = true;
      s_a.wasReleased = true;
      s_a.pressedAt   = millis() - 1500;  // > LONG_PRESS_MS so isLongPress check passes
      s_a.isPressed   = false;
    }
  }
}
#endif
```

- [ ] **Step 4: Gate `scanAxp()` on `!BOARD_HAS_KEY2`**

Replace the existing `scanAxp()` definition (lines 72-84) with the same body wrapped:

```cpp
#if !BOARD_HAS_KEY2
static void scanAxp() {
  if (hwExpanderAxpIrqLow()) {
    if (hwAxpPekeyShortPress()) s_axpEvt = 0x02;
    if (hwAxpPekeyLongPress())  s_axpEvt = 0x04;
  }
  bool pressed = (s_axpEvt == 0x02);
  s_b.wasPressed  = pressed;
  s_b.wasReleased = pressed;
  s_b.isPressed   = false;
  if (pressed) s_axpEvt = 0;
}
#endif
```

- [ ] **Step 5: Update `hwInputUpdate()` dispatcher to call the right scanners**

Replace lines 150-154 (`hwInputUpdate`) with:

```cpp
void hwInputUpdate() {
  scanKey1();
#if BOARD_HAS_KEY2
  scanKey2();
#else
  scanAxp();
#endif
#if BOARD_BTN_THIRD
  scanBootKey();
#endif
  scanTouch();
}
```

- [ ] **Step 6: Subtract `OFFSET_X/Y` in the non-letterbox touch remap**

Replace the non-letterbox branch in `scanTouch()` (lines 119-122) — the current code is:

```cpp
    #else
      s_tp.x = x[0] / 2;
      s_tp.y = y[0] / 2;
    #endif
```

Change to:

```cpp
    #else
      // Non-letterbox: physical → canvas via OFFSET subtract + 2× downscale.
      // OFFSET is 0 on 1.8 (full-fill), 56/16 on 2.16 (centred 368×448 in 480×480).
      int dx = x[0] - BOARD_DISPLAY_OFFSET_X;
      int dy = y[0] - BOARD_DISPLAY_OFFSET_Y;
      int tx = dx / 2;
      int ty = dy / 2;
      if (tx < 0) tx = 0; else if (tx >= BOARD_HW_W) tx = BOARD_HW_W - 1;
      if (ty < 0) ty = 0; else if (ty >= BOARD_HW_H) ty = BOARD_HW_H - 1;
      s_tp.x = tx;
      s_tp.y = ty;
    #endif
```

Repeat the same subtract-clamp for the FT3168 branch (line 138-141), changing:

```cpp
    s_tp.x = rx / 2;
    s_tp.y = ry / 2;
```

to:

```cpp
    int dx = rx - BOARD_DISPLAY_OFFSET_X;
    int dy = ry - BOARD_DISPLAY_OFFSET_Y;
    int tx = dx / 2;
    int ty = dy / 2;
    if (tx < 0) tx = 0; else if (tx >= BOARD_HW_W) tx = BOARD_HW_W - 1;
    if (ty < 0) ty = 0; else if (ty >= BOARD_HW_H) ty = BOARD_HW_H - 1;
    s_tp.x = tx;
    s_tp.y = ty;
```

(On 1.8 both offsets are 0, so this is byte-identical to the previous code after compile.)

- [ ] **Step 7: Build both S3 envs**

Run:
```bash
pio run -e waveshare-esp32s3-touch-amoled-1-8
pio run -e waveshare-esp32s3-touch-amoled-1-75c
```
Expected: both `[SUCCESS]`.

- [ ] **Step 8: Flash 1.8 and smoke-check button + touch**

- KEY1 short press → A event (e.g. clock advances mode if that's the binding, or menu confirms)
- AXP PEK short → B event (back / cancel)
- AXP PEK long → power off (S3 hardware-off behavior)
- Touch tap registers and the canvas-coord position is correct (e.g. tap on a buddy on the clock screen — the per-page handler should react)

- [ ] **Step 9: Flash 1.75C and smoke-check**

Same checks. The `BOARD_BTN_SWAP_AB=1` swap is unchanged, so the user-facing physical-right button is still A.

- [ ] **Step 10: Commit**

```bash
git add src/hw/input.cpp
git commit -m "input: KEY1 active-level flag, optional KEY2/BOOT-shortcut, offset-aware touch"
```

---

## Phase 2 — 2.16 Board

### Task 8: Vendor in the NimBLE-based ble_bridge

**Files:**
- Create: `src/ble_bridge_nimble.cpp` (copied from `~/Documents/claude-desktop-buddy-lcd/src/ble_bridge.cpp`)

- [ ] **Step 1: Copy the file**

```bash
cp /Users/yadongxie/Documents/claude-desktop-buddy-lcd/src/ble_bridge.cpp \
   /Users/yadongxie/Documents/claude-desktop-buddy/src/ble_bridge_nimble.cpp
```

- [ ] **Step 2: Verify the file's header includes**

Open `src/ble_bridge_nimble.cpp`. The first lines should be:

```cpp
#include "ble_bridge.h"
#include <NimBLEDevice.h>
#include <Arduino.h>
#include <string.h>
```

If not, fix to match. The file should expose the same public symbols (`bleInit`, `bleAvailable`, `bleRead`, `bleWrite`, `bleConnected`, `blePasskey`, `bleClearBonds`) that `ble_bridge.h` declares. The S3 `ble_bridge.cpp` and the LCD `ble_bridge.cpp` were API-compatible by design.

- [ ] **Step 3: Verify `ble_bridge.h` has not drifted between repos**

```bash
diff /Users/yadongxie/Documents/claude-desktop-buddy/src/ble_bridge.h \
     /Users/yadongxie/Documents/claude-desktop-buddy-lcd/src/ble_bridge.h
```

Expected: empty diff. If they differ, audit carefully — the LCD repo may have evolved the API. The S3 version is authoritative for this project; if a needed symbol is missing in the new file, port it back from the LCD repo's `.cpp`.

- [ ] **Step 4: Don't build yet — `build_src_filter` will exclude this file from S3 envs in the next task**

The file currently exists in `src/`. Without filter changes, `pio run -e waveshare-esp32s3-touch-amoled-1-8` would now compile both `ble_bridge.cpp` (Bluedroid) and `ble_bridge_nimble.cpp` (NimBLE), causing duplicate symbol link errors. **Hold the commit until Task 9 lands the filter** — or commit now without building, since the next task fixes the build.

- [ ] **Step 5: Commit (no build verification yet)**

```bash
git add src/ble_bridge_nimble.cpp
git commit -m "ble: vendor NimBLE NUS bridge from claude-desktop-buddy-lcd"
```

---

### Task 9: `platformio.ini` — `build_src_filter` per env, NimBLE only on 2.16

**Files:**
- Modify: `platformio.ini`

- [ ] **Step 1: Restructure `[env]` to exclude both BLE sources by default**

Change the existing `build_src_filter` line (currently
`+<*> +<buddies/> +<hw/> -<main.cpp.m5stick.bak> -<xfer.h>`) to also
exclude both BLE source files; each env then re-includes the one it wants:

```ini
build_src_filter =
    +<*>
    +<buddies/>
    +<hw/>
    -<main.cpp.m5stick.bak>
    -<xfer.h>
    -<ble_bridge.cpp>
    -<ble_bridge_nimble.cpp>
```

- [ ] **Step 2: Re-include `ble_bridge.cpp` (Bluedroid) on the two S3 envs**

Inside `[env:waveshare-esp32s3-touch-amoled-1-8]`, add:
```ini
build_src_filter = ${env.build_src_filter} +<ble_bridge.cpp>
```

Inside `[env:waveshare-esp32s3-touch-amoled-1-75c]`, add the same line.

- [ ] **Step 3: Remove the dead NimBLE dep from `[env]` lib_deps**

Delete the line `h2zero/NimBLE-Arduino @ ^1.4` from `[env]`. (S3 boards
have always used Bluedroid; the dep was unused.)

- [ ] **Step 4: Build both S3 envs to confirm Bluedroid path still works**

Run:
```bash
pio run -e waveshare-esp32s3-touch-amoled-1-8
pio run -e waveshare-esp32s3-touch-amoled-1-75c
```
Expected: both `[SUCCESS]`. If you see "undefined reference to bleInit" etc., the per-env `build_src_filter` line is wrong (it must use `${env.build_src_filter}` to inherit, not replace).

- [ ] **Step 5: Flash 1.8, BLE smoke test**

Boot the board, pair from the desktop Hardware Buddy, send a short message. Verify pair + message work. The Bluedroid bonding NVS is unchanged so existing bonds carry over.

- [ ] **Step 6: Commit**

```bash
git add platformio.ini
git commit -m "pio: drop dead NimBLE-1.4 dep; per-env BLE source via filter"
```

---

### Task 10: Create the 2.16 board header

**Files:**
- Create: `src/boards/board_waveshare_esp32c6_touch_amoled_2_16.h`

- [ ] **Step 1: Write the header**

```cpp
// src/boards/board_waveshare_esp32c6_touch_amoled_2_16.h
#pragma once

// Display: SH8601 480×480 rounded-square AMOLED, 2.16" diagonal.
// 184×224 canvas → 2× integer upscale → 368×448, centred at (56, 16).
// Rounded corners fall in the 56 px L/R black border, never clipping content.
#define LCD_W_PHYS              480
#define LCD_H_PHYS              480
#define BOARD_HW_W              184
#define BOARD_HW_H              224
#define BOARD_SAFE_INSET        8
#define BOARD_DISPLAY_OFFSET_X  56
#define BOARD_DISPLAY_OFFSET_Y  16

// QSPI to SH8601 (per XiaoZhi v2.2.5 board def + schematic verification)
#define PIN_LCD_SDIO0  1
#define PIN_LCD_SDIO1  2
#define PIN_LCD_SDIO2  3
#define PIN_LCD_SDIO3  4
#define PIN_LCD_SCLK   0
#define PIN_LCD_CS     15
// PIN_LCD_RESET intentionally undefined — panel reset is via AXP ALDO3 power-cycle.

// I2C bus (shared: AXP2101, ES8311, ES7210, CST9217, QMI8658, PCF85063)
#define PIN_I2C_SDA   8
#define PIN_I2C_SCL   7

// Touch: CST9217 (CST92xx family, same protocol path as 1.75C's CST92xx)
#define PIN_TP_INT    5
#define PIN_TP_RESET  11

// I2S to ES8311 codec
#define PIN_I2S_MCLK  19
#define PIN_I2S_BCLK  20
#define PIN_I2S_WS    22
#define PIN_I2S_DI    21
#define PIN_I2S_DO    23
#define PIN_PA_CTRL   -1   // No discrete PA enable on 2.16; gated by BOARD_HAS_PA_CTRL=0.

// IMU (QMI8658, polled — INTs reserved but not wired to handlers)
#define PIN_QMI_INT1  16
#define PIN_QMI_INT2  17

// Three physical keys
#define PIN_KEY1      18   // PWR silkscreen. Active-HIGH via BSS138 inverter. Also AXP PWRON.
#define PIN_KEY2      10   // IO10 silkscreen. Active-low.
#define PIN_KEY_BOOT  9    // BOOT silkscreen. Active-low. Synthesises BTN_A_LONG_PRESS.

// Capability flags
#define BOARD_HAS_PSRAM            0
#define BOARD_HAS_TCA9554          0
#define BOARD_HAS_PCF85063         1
#define BOARD_HAS_PA_CTRL          0
#define BOARD_HAS_AXP2101          1
#define BOARD_LCD_RST_VIA_PMU      1
#define BOARD_AXP_PWRON_4S_OFF     1
#define BOARD_DISPLAY_CO5300       0
#define BOARD_DISPLAY_LETTERBOX    0
#define BOARD_TOUCH_CST92XX        1
#define BOARD_BTN_SWAP_AB          0
#define BOARD_BTN_THIRD            1
#define BOARD_KEY1_ACTIVE_HIGH     1
#define BOARD_HAS_KEY2             1

// Credits page
#define BOARD_MODEL_LINE1  "Waveshare ESP32-C6"
#define BOARD_MODEL_LINE2  "Touch AMOLED 2.16"
```

- [ ] **Step 2: Don't build yet — pins.h / platformio.ini still need to know about this board**

Hold the commit until Task 12 (env wiring) completes; until then the header is dead code.

- [ ] **Step 3: Commit**

```bash
git add src/boards/board_waveshare_esp32c6_touch_amoled_2_16.h
git commit -m "boards: add esp32c6-touch-amoled-2.16 header"
```

---

### Task 11: Add `pins.h` dispatcher branch for 2.16

**Files:**
- Modify: `src/hw/pins.h`

- [ ] **Step 1: Add the `#elif` branch**

Open `src/hw/pins.h`. After the existing `_1_75C` branch and before the `#else` error, insert:

```cpp
#elif defined(BOARD_WAVESHARE_ESP32C6_TOUCH_AMOLED_2_16)
  #include "../boards/board_waveshare_esp32c6_touch_amoled_2_16.h"
```

- [ ] **Step 2: Update the `#error` message to mention the new env**

Change the `#error` line to:

```cpp
#error "No board defined. Add -DBOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_8, _1_75C, or _ESP32C6_TOUCH_AMOLED_2_16 to build_flags in platformio.ini."
```

- [ ] **Step 3: Build both S3 envs (the 2.16 env doesn't exist yet)**

Run:
```bash
pio run -e waveshare-esp32s3-touch-amoled-1-8
pio run -e waveshare-esp32s3-touch-amoled-1-75c
```
Expected: both `[SUCCESS]`.

- [ ] **Step 4: Commit**

```bash
git add src/hw/pins.h
git commit -m "pins: dispatcher branch for esp32c6-touch-amoled-2.16"
```

---

### Task 12: Add `[env:waveshare-esp32c6-touch-amoled-2-16]` block

**Files:**
- Modify: `platformio.ini`

- [ ] **Step 1: Append the new env block**

At the end of `platformio.ini`, after the `1_75c` block, add:

```ini
[env:waveshare-esp32c6-touch-amoled-2-16]
board = esp32-c6-devkitc-1
board_build.mcu = esp32c6
board_build.f_cpu = 160000000L
board_build.flash_mode = qio
; No psram_type, no arduino.memory_type — C6 has no PSRAM.
build_flags =
    -DCORE_DEBUG_LEVEL=0
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DXPOWERS_CHIP_AXP2101
    -DU8G2_USE_LARGE_FONTS
    -DU8G2_FONT_SUPPORT
    -DBOARD_WAVESHARE_ESP32C6_TOUCH_AMOLED_2_16
build_src_filter = ${env.build_src_filter} +<ble_bridge_nimble.cpp>
lib_deps =
    ${env.lib_deps}
    h2zero/NimBLE-Arduino @ ^2.0
lib_ignore =
    Adafruit_XCA9554
    Arduino_DriveBus
```

`lib_ignore` keeps the S3-only vendored libs out of the C6 build — saves
flash and avoids platform-incompatible includes.

- [ ] **Step 2: Override the global S3-only board defaults the env inherits**

The `[env]` section currently has:
```ini
board = esp32-s3-devkitc-1
board_build.mcu = esp32s3
board_build.f_cpu = 240000000L
board_build.psram_type = opi
board_build.arduino.memory_type = qio_opi
```

Move the four S3-specific lines (`board`, `board_build.mcu`,
`board_build.f_cpu`, `board_build.psram_type`,
`board_build.arduino.memory_type`) **out of `[env]` and into each S3 env
block individually**. The 2.16 env already overrides `board`, `mcu`,
`f_cpu`; this prevents PIO from feeding C6 builds the wrong `psram_type`
and `arduino.memory_type` that would either fail or silently mis-link.

After the move, `[env:waveshare-esp32s3-touch-amoled-1-8]` and
`[env:waveshare-esp32s3-touch-amoled-1-75c]` should each contain:
```ini
board = esp32-s3-devkitc-1
board_build.mcu = esp32s3
board_build.f_cpu = 240000000L
board_build.psram_type = opi
board_build.arduino.memory_type = qio_opi
```
in addition to their existing `build_flags` and `build_src_filter`.

- [ ] **Step 3: Build all three envs**

Run:
```bash
pio run -e waveshare-esp32s3-touch-amoled-1-8
pio run -e waveshare-esp32s3-touch-amoled-1-75c
pio run -e waveshare-esp32c6-touch-amoled-2-16
```
Expected: all three `[SUCCESS]`. The 2.16 build may emit warnings about
`Adafruit_XCA9554` / `Arduino_DriveBus` being unused — `lib_ignore` should
suppress them; if not, those warnings are benign.

- [ ] **Step 4: Note any 2.16 link errors**

If the 2.16 link fails on missing symbols (e.g., `hwExpanderResetSequence`
referenced by `hw.cpp` even though `BOARD_LCD_RST_VIA_PMU=1` should gate
it out), the gate is wrong — re-check Task 6's `#if !BOARD_LCD_RST_VIA_PMU`
guard around the call. The function declaration in `expander.h` is still
visible regardless of `BOARD_HAS_TCA9554`; the call site is what matters.

If link fails on `Adafruit_XCA9554` headers transitively, check that
`hw/expander.cpp` 's `#if BOARD_HAS_TCA9554` guard is around the
`Adafruit_XCA9554` include too, not just the function bodies.

- [ ] **Step 5: Commit**

```bash
git add platformio.ini
git commit -m "pio: add esp32c6-touch-amoled-2.16 env"
```

---

### Task 13: First flash — verify panel lights up and full-screen colour fills work

**Files:** none (firmware test)

- [ ] **Step 1: Find the device**

Plug the 2.16 board in, then:
```bash
ls /dev/cu.usbmodem*
```
Expected: a single port. Note the path.

- [ ] **Step 2: Flash**

```bash
pio run -e waveshare-esp32c6-touch-amoled-2-16 -t upload --upload-port /dev/cu.usbmodem21101
```
(replace the port if different).

- [ ] **Step 3: Open serial monitor**

```bash
pio device monitor -e waveshare-esp32c6-touch-amoled-2-16
```

Expected boot log lines (in any order):
- `=== claude-buddy waveshare boot ===`
- `hwInit OK`

If you see `hwInit FAIL: display`, the SH8601 init likely needs the LVGL
demo's vendor cmd table (Risk #2 in the spec). Skip to Task 14.

- [ ] **Step 4: Visually verify the panel**

The panel should light up showing the boot UI (clock or first-pair
screen). The 184×224 canvas should land in the centre of the 480×480
screen with 56 px black border on each side and 16 px on top/bottom.
Rounded corners should be entirely inside the black border.

Common visual failures and what they mean:
- Garbled / pixelated → SH8601 init mismatch — see Task 14.
- Image visible but offset wrong → `BOARD_DISPLAY_OFFSET_X/Y` value wrong.
- Image visible but mirrored → set the SH8601 `MADCTL` (reg 0x36) bits or
  flag a future `BOARD_DISPLAY_MIRROR_X/Y`. The XiaoZhi panel def uses
  `0x36 = 0xA0`, the LVGL demo uses `0x36 = 0x30`. Try the LVGL value
  first (it matches the 0..479 init window).
- Panel dim / partial light → AXP ALDO3 not actually powered on. Check
  `s_pmu.enableALDO3()` in `powerInit()` and the toggle in `hwInit()`.

- [ ] **Step 5: If all checks pass, commit a marker**

```bash
git commit --allow-empty -m "smoke: 2.16 panel lit, canvas centred at (56, 16)"
```

---

### Task 14: (Conditional) Switch to vendor SH8601 init if Arduino_SH8601 fails

**Files (only if Task 13 step 4 showed garbled output):**
- Create: `lib/esp_lcd_sh8601_2_16/` from the LVGL demo
- Modify: `src/hw/display.cpp` to use the IDF `esp_lcd_panel_sh8601` path on `BOARD_LCD_RST_VIA_PMU=1`

This task is **only executed if the SH8601 panel does not initialise via
`Arduino_SH8601` on the 2.16**. If Task 13 step 4 passed cleanly, skip
straight to Task 15.

- [ ] **Step 1: Vendor the LVGL demo's SH8601 driver**

```bash
mkdir -p /Users/yadongxie/Documents/claude-desktop-buddy/lib/esp_lcd_sh8601_2_16
cp /Users/yadongxie/Downloads/ESP32-C6-Touch-AMOLED-2.16/02_Example/Arduino-v3.3.3/08_LVGL_V8_Test/src/externLib/esp_lcd_sh8601.{c,h} \
   /Users/yadongxie/Documents/claude-desktop-buddy/lib/esp_lcd_sh8601_2_16/
```

- [ ] **Step 2: In `display.cpp`, gate an alternate init path**

Add at the top of `hwDisplayInit()`:

```cpp
#if BOARD_LCD_RST_VIA_PMU
  // 2.16: Arduino_SH8601 doesn't init this panel revision. Use the IDF
  // esp_lcd_panel_sh8601 component with the LVGL demo's vendor cmd table.
  return hwDisplayInit_SH8601_IDF();
#endif
```

Then add `hwDisplayInit_SH8601_IDF()` (full code: see
`~/Downloads/ESP32-C6-Touch-AMOLED-2.16/02_Example/Arduino-v3.3.3/08_LVGL_V8_Test/bsp_lvgl_port.cpp:127-207` —
copy the spi_bus + esp_lcd_new_panel_io_spi + esp_lcd_new_panel_sh8601
sequence; init cmds at line 25-41 of that file). The function returns
`true` on success and `false` on failure.

`s_gfx` is not used in this path; export the panel handle from
`display.cpp` and have `hwDisplayPush()` call
`esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2+1, y2+1, buf)` instead
of `s_gfx->draw16bitRGBBitmap`. Behind the same
`BOARD_LCD_RST_VIA_PMU` flag. Keep the bilinear / 2× logic identical.

- [ ] **Step 3: Build, flash, and re-verify**

```bash
pio run -e waveshare-esp32c6-touch-amoled-2-16 -t upload
```
Visual check: panel lights up cleanly, canvas centred.

- [ ] **Step 4: Commit**

```bash
git add lib/esp_lcd_sh8601_2_16/ src/hw/display.cpp
git commit -m "display: vendor SH8601 IDF driver for 2.16 panel revision"
```

---

### Task 15: Smoke test — three buttons + touch

**Files:** none

- [ ] **Step 1: Verify each button independently**

With the device booted to the clock screen:
- Press PWR (middle key) briefly → A action (e.g. species cycle, depending on screen binding)
- Press IO10 (left key) briefly → B action (e.g. transcript scroll / back)
- Press BOOT (right key) briefly → menu opens (synthesised long-press A)
- Hold PWR for 4 s → device powers off (AXP cuts ALDO3)
- Press PWR briefly when off → device powers back on, bond / state restored

If A and B are inverted (PWR triggers B, IO10 triggers A): the active-level
flag is wrong; flip `BOARD_KEY1_ACTIVE_HIGH` and rebuild.

- [ ] **Step 2: Verify touch**

- Tap upper half of the active canvas region on a screen with no specific
  binding → no event (correct).
- Tap inside black border (e.g. far left edge) → no event (correct;
  out-of-canvas taps are clamped).
- Open menu, tap a menu item → that item is selected.
- Swipe up / down on the clock screen → display mode cycles.
- Swipe left / right on the clock screen → species cycles.

If swipe gestures fire but coordinates are mirrored, check `s_cst.setMirrorXY()`
in `input.cpp` — mirror-X / Y values may need flipping for the 2.16 panel
orientation. (1.75C uses `(true, true)`. The 2.16 may need different.)

- [ ] **Step 3: Commit empty marker**

```bash
git commit --allow-empty -m "smoke: 2.16 three keys + touch + 4s power-off OK"
```

---

### Task 16: Smoke test — BLE pairing, message send/receive, attention HUD

**Files:** none

- [ ] **Step 1: Pair from desktop**

Open the Hardware Buddy desktop app. It should advertise a Claude-XXXX
device. Connect; the 2.16 should display a 6-digit passkey. Type it on
desktop. Bond completes.

If pairing hangs at passkey: Check NimBLE security callback wiring in
`ble_bridge_nimble.cpp`. The C6-LCD repo's commit `5899ca7` is the working
reference.

- [ ] **Step 2: Send a long message**

Trigger the desktop to send a multi-line CJK message (say, a 200-character
zh-Hans chat snippet). Verify it renders in the transcript area, scrollable
with B button or touch.

- [ ] **Step 3: Approval flow**

Trigger an approval prompt from desktop. The 2.16 should pulse the red
attention HUD. Tap upper half of canvas → approve. Tap lower half →
deny. Confirm desktop receives the response.

- [ ] **Step 4: Bond persistence**

Power off via PWR 4 s. Power back on. Reconnect from desktop without
re-entering passkey. If passkey is required again, NimBLE bonding store
isn't surviving NVS — check that `bleClearBonds()` isn't being called on
boot, and that NVS partition is correctly set in `partitions = no_ota_8mb.csv`.

- [ ] **Step 5: Commit empty marker**

```bash
git commit --allow-empty -m "smoke: 2.16 BLE pair + long msg + approval + bond persistence OK"
```

---

### Task 17: Smoke test — IMU, RTC, audio beep

**Files:** none

- [ ] **Step 1: IMU**

Shake the device. The dizzy animation should play. Tilt it sideways: the
buddy should react if the tilt gesture is wired (see `main.cpp` IMU
handlers). If nothing happens, check `hwImuInit()` returned true in serial
log; QMI8658 at 0x6B should be detected.

- [ ] **Step 2: RTC**

Open the info / settings screen showing time. The clock should tick once
per second, surviving reboot (PCF85063 holds time across resets). If the
time resets on reboot, the PCF85063 has no battery backup or the I²C
write isn't sticking — verify `hwRtcInit()` and time-set call sites.

- [ ] **Step 3: Audio**

Trigger any in-app sound (open menu, navigate to a setting that beeps).
Verify the speaker plays. If silent: ES8311 likely failed to init —
check `hwAudioInit()` serial log for `hwAudio: ES8311 init failed`.

- [ ] **Step 4: Commit empty marker**

```bash
git commit --allow-empty -m "smoke: 2.16 IMU + RTC + audio beep OK"
```

---

### Task 18: README — document the new board

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Add the 2.16 row to the supported-boards table**

Find the existing supported-boards section (the table currently lists 1.8
and 1.75C). Add a row:

```markdown
| Waveshare ESP32-C6-Touch-AMOLED-2.16 | `waveshare-esp32c6-touch-amoled-2-16` | 480×480 SH8601 (rounded square), AXP2101, ES8311, PCF85063, QMI8658, CST9217 — three physical keys |
```

- [ ] **Step 2: Add controls matrix entry for 2.16**

In the controls section, add:

```markdown
### ESP32-C6-Touch-AMOLED-2.16 controls

- **PWR** (middle physical key) — primary action / confirm
- **IO10** (left key) — secondary / back / scroll
- **BOOT** (right key) — open menu (shortcut)
- **PWR held 4 s** — power off (AXP cuts power; press again to wake)
- **Touch** — gestures and approval upper-/lower-half tap
```

- [ ] **Step 3: Add pin reference table for 2.16**

Add a section similar to the existing 1.8 / 1.75C pin tables; copy the
header from `src/boards/board_waveshare_esp32c6_touch_amoled_2_16.h`.

- [ ] **Step 4: Build verification**

`README.md` doesn't affect build. No build needed.

- [ ] **Step 5: Commit**

```bash
git add README.md
git commit -m "readme: document esp32c6-touch-amoled-2.16"
```

---

### Task 19: Final regression — rebuild and flash 1.8 / 1.75C end-to-end

**Files:** none

- [ ] **Step 1: Build all three envs from a clean state**

```bash
pio run -t clean
pio run -e waveshare-esp32s3-touch-amoled-1-8
pio run -e waveshare-esp32s3-touch-amoled-1-75c
pio run -e waveshare-esp32c6-touch-amoled-2-16
```
Expected: all three `[SUCCESS]`.

- [ ] **Step 2: Flash 1.8 and run a full smoke test**

- Boot, panel lights up
- KEY1 / AXP PEK both register A / B events
- Touch tap registers
- BLE pair, long message
- Audio beep
- Approval flow

- [ ] **Step 3: Flash 1.75C and run the same**

Same checks. (`BOARD_BTN_SWAP_AB=1` swap is preserved.)

- [ ] **Step 4: Flash 2.16 and run the same**

Three keys, touch, BLE, audio, approval, IMU, RTC.

- [ ] **Step 5: If all three boards pass, mark the port complete**

```bash
git commit --allow-empty -m "smoke: full regression on 1.8 / 1.75C / 2.16 — port complete"
```

---

## Self-Review

**Spec coverage check:**

| Spec section | Plan task |
|---|---|
| New flag declarations on existing boards | Tasks 1, 2 |
| `display.cpp` OFFSET_X/Y reads | Task 3 |
| `audio.cpp` PA gating | Task 4 |
| `power.cpp` ALDO3 enable + PWRON 4s gating | Task 5 |
| `hw.cpp` LCD reset gating | Task 6 |
| `input.cpp` KEY1 active level + KEY2 + BOOT shortcut + scanAxp gate + touch offset | Task 7 |
| 2.16 board header | Task 10 |
| pins.h dispatcher branch | Task 11 |
| `[env]` block for 2.16 + per-env src filter | Tasks 9, 12 |
| NimBLE port for 2.16 | Tasks 8, 9 |
| SH8601 init fallback (Risk #2) | Task 14 (conditional) |
| Hardware smoke tests | Tasks 13, 15, 16, 17 |
| README update | Task 18 |
| Final regression | Task 19 |

All spec sections covered.

**Placeholder scan:** Searched for "TBD", "TODO", "implement later" — none. Code blocks contain actual content; tasks reference exact files and line numbers.

**Type / name consistency:**
- `BOARD_HAS_KEY2` defined Task 1, used Task 7 ✓
- `BOARD_KEY1_ACTIVE_HIGH` defined Task 1, used Task 7 ✓
- `BOARD_DISPLAY_OFFSET_X/Y` defined Task 1, used Tasks 3, 7 ✓
- `BOARD_LCD_RST_VIA_PMU` defined Task 1, used Task 6 ✓
- `BOARD_AXP_PWRON_4S_OFF` defined Task 1, used Task 5 ✓
- `hwPmuRef()` declared Task 6, called Task 6 ✓
- `PIN_KEY1`, `PIN_KEY2`, `PIN_KEY_BOOT` defined Task 10, read Task 7 — consistent ✓
- `s_a`, `s_b` populated by `scanKey1` / `scanKey2` / `scanAxp` — Task 7 dispatcher routes correctly ✓

All cross-task references resolve.
