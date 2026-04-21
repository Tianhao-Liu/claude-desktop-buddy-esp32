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
