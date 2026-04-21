# Port Design: Waveshare ESP32-S3-Touch-AMOLED-1.75C

**Date:** 2026-04-21
**Status:** Approved

## Background

The existing codebase targets the Waveshare ESP32-S3-Touch-AMOLED-1.8 (rectangular AMOLED, 368×448 physical pixels). This spec covers porting the same buddy codebase to the Waveshare ESP32-S3-Touch-AMOLED-1.75C, which has a round AMOLED display at 466×466 pixels. Both boards use the same ESP32-S3R8 MCU.

## Goals

- Run the existing buddy on the 1.75C without duplicating `main.cpp` or `buddies/`
- Establish a multi-board config system that makes future board ports a single-header addition
- Adapt the safe area for the round display so content is not clipped by the circle

## Out of Scope

- Redesigning the UI for the circular form factor
- Adding drivers for board-exclusive peripherals (QMI8658C IMU, SYS_OUT signal)
- Using LCD_TE tearing-effect synchronisation

---

## Architecture

### Multi-board Config System

A new `src/boards/` directory holds one header per board. Each header declares all hardware constants for that board. `hw/pins.h` becomes a dispatcher that `#include`s the right header based on a build flag set in `platformio.ini`.

```
src/
  boards/
    board_waveshare_esp32s3_touch_amoled_1_8.h
    board_waveshare_esp32s3_touch_amoled_1_75c.h
  hw/
    pins.h        ← dispatcher: #if BOARD_... → #include boards/...
    display.h     ← constants come from board header, not hardcoded
    display.cpp   ← no changes needed
    expander.h/cpp ← #if BOARD_HAS_TCA9554 splits two implementations
    input.cpp     ← AXP IRQ read adapts to board capability
  buddies/        ← untouched
  main.cpp        ← untouched
```

### PlatformIO Environments

```ini
[env:waveshare-esp32s3-touch-amoled-1-8]
build_flags = ... -DBOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_8

[env:waveshare-esp32s3-touch-amoled-1-75c]
build_flags = ... -DBOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_75C
```

Adding a future board = one new header in `src/boards/` + one new env in `platformio.ini` + one `#elif` in `hw/pins.h`. `main.cpp` and `buddies/` never need touching.

---

## Hardware Differences: 1.8 vs 1.75C

Derived from the ESP32-S3-Touch-AMOLED-1.75C schematic (1.75C-schematic).

### GPIO Pin Mapping

| Signal | 1.8 | 1.75C | Note |
|---|---|---|---|
| `PIN_LCD_SCLK` | GPIO11 | GPIO38 | |
| `PIN_LCD_RESET` | EXIO0 (TCA9554) | GPIO1 | Now direct GPIO |
| `PIN_TP_INT` | GPIO21 | GPIO11 | |
| `PIN_TP_RESET` | EXIO1 (TCA9554) | GPIO2 | Now direct GPIO |
| `PIN_LCD_TE` | — | GPIO13 | New; not used yet |
| DSI_PWR_EN | EXIO2 | — | Removed |
| `PIN_LCD_CS` | GPIO12 | GPIO12 | Same |
| `PIN_LCD_SDIO0–3` | GPIO4–7 | GPIO4–7 | Same |
| `PIN_I2C_SCL` | GPIO14 | GPIO14 | Same |
| `PIN_I2C_SDA` | GPIO15 | GPIO15 | Same |
| `PIN_I2S_MCLK` | GPIO16 | GPIO16 | Same |
| `PIN_I2S_BCLK` | GPIO9 | GPIO9 | Same |
| `PIN_I2S_WS` | GPIO45 | GPIO45 | Same |
| `PIN_I2S_DI` | GPIO10 | GPIO10 | Same |
| `PIN_I2S_DO` | GPIO8 | GPIO8 | Same |
| `PIN_PA_CTRL` | GPIO46 | GPIO46 | Same |
| `PIN_KEY1` | GPIO0 | GPIO0 | Same |

### Board Capabilities

| Constant | 1.8 | 1.75C |
|---|---|---|
| `BOARD_HAS_TCA9554` | 1 | 0 |
| AXP IRQ | via TCA9554 EXIO5 | not wired to any GPIO; poll via I2C |

### Display

| Constant | 1.8 | 1.75C |
|---|---|---|
| `LCD_W_PHYS` | 368 | 466 |
| `LCD_H_PHYS` | 448 | 466 |
| `HW_W` (logical canvas) | 184 | 233 |
| `HW_H` (logical canvas) | 224 | 233 |
| `SAFE_INSET` | 8 | 35 (calculated; calibrate on device) |
| Shape | Rectangle | Circle |

The 1.75C display is physically circular at 466×466. The logical canvas is 233×233 (2× upscale). SAFE_INSET of 35 px yields a 163×163 usable rectangle fully inside the inscribed circle (calculated: 233/2 × √2 ≈ 164). Final value to be confirmed with DEBUG_SAFE_BOX on device.

Display driver chip: assumed SH8601 (same as 1.8); to confirm on first boot.

---

## File-by-File Changes

### `platformio.ini`

- Rename existing `[env:waveshare-amoled]` → `[env:waveshare-esp32s3-touch-amoled-1-8]`
- Add `[env:waveshare-esp32s3-touch-amoled-1-75c]` with same base settings plus new board flag
- Both envs share `lib_deps`, `board`, `upload_speed`, etc. via a `[env]` base section

### `src/boards/board_waveshare_esp32s3_touch_amoled_1_8.h` (new)

Extract all constants currently in `hw/pins.h` and `hw/display.h`. Name them with the exact identifiers those files already use, so no other file needs changing:
- All `PIN_*` defines
- `LCD_W_PHYS`, `LCD_H_PHYS`
- `HW_W`, `HW_H`, `SAFE_INSET` (display.h will continue to expose these as `constexpr int`)
- `BOARD_HAS_TCA9554 1`
- Expander pin constants (`EXIO_LCD_RESET`, `EXIO_TP_RESET`, `EXIO_DSI_PWR_EN`, `EXIO_AXP_IRQ`)

### `src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h` (new)

Same set of identifiers as the 1.8 header so every consumer compiles unchanged:
- All `PIN_*` defines with 1.75C values
- `LCD_W_PHYS 466`, `LCD_H_PHYS 466`
- `HW_W 233`, `HW_H 233`, `SAFE_INSET 35`
- `BOARD_HAS_TCA9554 0`
- `PIN_LCD_RESET 1`, `PIN_TP_RESET 2`, `PIN_LCD_TE 13`
- No `EXIO_*` constants (no TCA9554)

### `src/hw/pins.h`

Replace current content with a dispatcher:
```cpp
#if defined(BOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_8)
  #include "../boards/board_waveshare_esp32s3_touch_amoled_1_8.h"
#elif defined(BOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_75C)
  #include "../boards/board_waveshare_esp32s3_touch_amoled_1_75c.h"
#else
  #error "No board defined. Set BOARD_... in platformio.ini build_flags."
#endif
```

### `src/hw/display.h`

Board headers define display dimensions with a `BOARD_` prefix to avoid a self-referential `constexpr`. `display.h` wraps them with the names the rest of the codebase already uses:

```cpp
// board header defines: BOARD_HW_W, BOARD_HW_H, BOARD_LCD_W_PHYS, BOARD_LCD_H_PHYS, BOARD_SAFE_INSET
constexpr int HW_W      = BOARD_HW_W;
constexpr int HW_H      = BOARD_HW_H;
constexpr int LCD_W_PHYS = BOARD_LCD_W_PHYS;
constexpr int LCD_H_PHYS = BOARD_LCD_H_PHYS;
constexpr int SAFE_INSET = BOARD_SAFE_INSET;
// SAFE_L / SAFE_T / SAFE_R / SAFE_B / SAFE_W / SAFE_H derived as today
```

`PIN_*` constants and `EXIO_*` constants keep their existing names in the board header (no prefix) since `pins.h` always consumed them with those names.

### `src/hw/expander.cpp`

Split reset sequence on `BOARD_HAS_TCA9554`:

```cpp
void hwExpanderResetSequence() {
#if BOARD_HAS_TCA9554
  // existing TCA9554 logic
#else
  digitalWrite(PIN_LCD_RESET, LOW);
  digitalWrite(PIN_TP_RESET,  LOW);
  delay(20);
  digitalWrite(PIN_LCD_RESET, HIGH);
  digitalWrite(PIN_TP_RESET,  HIGH);
  delay(20);
#endif
}
```

`hwExpanderInit()` on 1.75C: configure `PIN_LCD_RESET` and `PIN_TP_RESET` as `OUTPUT`, skip TCA9554 `begin()`.

`hwExpanderAxpIrqLow()` on 1.75C: always return `true` (AXP_IRQ unrouted → poll every frame via I2C; has negligible cost).

### `src/hw/input.cpp`

No structural changes needed. `scanAxp()` already calls `hwExpanderAxpIrqLow()` which will return `true` on 1.75C, causing AXP PEK registers to be read every call. This is acceptable.

---

## Open Items (Verify on Device)

| Item | Expected | Action |
|---|---|---|
| `SAFE_INSET` value | 35 px | Run with `DEBUG_SAFE_BOX`, adjust |
| Display driver chip | SH8601 | Confirm `s_gfx->begin()` succeeds |
| QMI_INT1 actual GPIO | GPIO21 (table alignment suspect) | Check with multimeter or logic analyser |
| AXP_IRQ wiring | Unrouted | If IRQ-driven behaviour is needed later, trace net on board |
