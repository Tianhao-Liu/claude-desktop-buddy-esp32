// src/boards/board_waveshare_esp32s3_touch_amoled_1_8_v2.h
//
// Waveshare ESP32-S3-Touch-AMOLED-1.8 *hardware V2*. Identical board layout to
// the V1 header (board_waveshare_esp32s3_touch_amoled_1_8.h) — same QSPI pins,
// same TCA9554 expander, same AXP2101 / PCF85063 / QMI8658 / ES8311 — EXCEPT:
//   * display driver is CO5300 (V1 was SH8601); RAM window starts at column 16
//   * touch controller is CST816 (V1 was FT3168)
// Both drivers are vendored in lib/Arduino_DriveBus. The version is printed on a
// label on the back of the board.
#pragma once

// Display physical resolution (CO5300 AMOLED, 2× upscaled from canvas).
#define LCD_W_PHYS  368
#define LCD_H_PHYS  448

// Logical canvas (half of physical — upscale done in hwDisplayPush).
#define BOARD_HW_W        184
#define BOARD_HW_H        224

// Safe draw area: symmetric inset from bezel corners.
#define BOARD_SAFE_INSET  8

// QSPI to CO5300
#define PIN_LCD_SDIO0  4
#define PIN_LCD_SDIO1  5
#define PIN_LCD_SDIO2  6
#define PIN_LCD_SDIO3  7
#define PIN_LCD_SCLK   11
#define PIN_LCD_CS     12
// LCD_RESET and TP_RESET are driven via TCA9554 expander (see EXIO_* below)

// I2C bus (shared: TCA9554, AXP2101, CST816, ES8311, PCF85063, QMI8658)
#define PIN_I2C_SDA   15
#define PIN_I2C_SCL   14

// CST816 touch interrupt — direct to ESP32
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

// Display: Arduino_CO5300. CO5300 panels can't tolerate per-row blits (QSPI
// state desyncs and the screen stays black — same issue documented on the
// S3-2.16). So reuse the LETTERBOX path: PSRAM full-frame buffer + one-shot
// draw16bitRGBBitmap. Here DEST == physical (368×448 = exact 2× of 184×224),
// so the centring offset is (0,0) and there's no actual black border.
#define BOARD_DISPLAY_CO5300     1
#define BOARD_DISPLAY_LETTERBOX  1
#define BOARD_DISPLAY_DEST_W     368   // 184 × 2 (exact integer upscale)
#define BOARD_DISPLAY_DEST_H     448   // 224 × 2

// Touch: CST816 @ 0x15 via Arduino_DriveBus (Arduino_CST816x)
#define BOARD_TOUCH_CST92XX  0
#define BOARD_TOUCH_CST816   1

#define BOARD_BTN_SWAP_AB  0

// External PCF85063 RTC @ 0x51 on the I2C bus
#define BOARD_HAS_PCF85063  1

#define BOARD_HAS_PSRAM            1
#define BOARD_DISPLAY_OFFSET_X     0
#define BOARD_DISPLAY_OFFSET_Y     0
#define BOARD_DISPLAY_SCALE        2
#define BOARD_HAS_PA_CTRL          1
#define BOARD_HAS_AXP2101          1
#define BOARD_LCD_RST_VIA_PMU      0
#define BOARD_AXP_PWRON_4S_OFF     0
#define BOARD_BTN_THIRD            0
#define BOARD_KEY1_ACTIVE_HIGH     0
#define BOARD_HAS_KEY2             0

// Credits-page hardware identification (two short lines).
#define BOARD_MODEL_LINE1  "Waveshare ESP32-S3"
#define BOARD_MODEL_LINE2  "Touch AMOLED 1.8 V2"

#define BOARD_DISPLAY_SH8601_VENDOR_INIT  0
#define BOARD_AXP_ENABLE_AUX_LDOS  0
#define BOARD_DISPLAY_PUSH_STREAMED  0   // per-row 2× upscale works fine on S3 240 MHz
#define BOARD_CO5300_COL_OFFSET    16  // V2 1.8 panel RAM window starts at column 16
#define BOARD_DISPLAY_ROTATION     0
#define BOARD_CO5300_MADCTL        0   // 0 = use lib's setRotation MADCTL unchanged
