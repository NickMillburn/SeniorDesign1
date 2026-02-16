#include <Wire.h>
#include <U8g2lib.h>
#include <DallasTemperature.h>
#include "display.h"

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void display_init() {
  Wire.begin();
  u8g2.begin();
}

void display_update(bool btn1, float tempC1, bool btn2, float tempC2) {
  char line[32];
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_tf);

  // Sensor 1 — top half
  if (!btn1) {
    u8g2.drawStr(0, 14, "Sensor 1 off");
  } else if (tempC1 == DEVICE_DISCONNECTED_C) {
    u8g2.drawStr(0, 14, "Sensor 1 ERROR");
  } else {
    snprintf(line, sizeof(line), "S1: %.1f C", (double)tempC1);
    u8g2.drawStr(0, 14, line);
  }

  // Divider line
  u8g2.drawHLine(0, 32, 128);

  // Sensor 2 — bottom half
  if (!btn2) {
    u8g2.drawStr(0, 50, "Sensor 2 off");
  } else if (tempC2 == DEVICE_DISCONNECTED_C) {
    u8g2.drawStr(0, 50, "Sensor 2 ERROR");
  } else {
    snprintf(line, sizeof(line), "S2: %.1f C", (double)tempC2);
    u8g2.drawStr(0, 50, line);
  }

  u8g2.sendBuffer();
}

// Function to display temperature readings from two sensors on the OLED display
//UNTESTED
void display_show_temperature(float temp1, float temp2) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_logisoso32_tf);

  char tempStr[32];
  snprintf(tempStr, sizeof(tempStr), "T1: %.1fC", temp1);
  int16_t x1 = (128 - u8g2.getStrWidth(tempStr)) / 2;
  int16_t y1 = 30;

  snprintf(tempStr, sizeof(tempStr), "T2: %.1fC", temp2);
  int16_t x2 = (128 - u8g2.getStrWidth(tempStr)) / 2;
  int16_t y2 = 60;

  u8g2.drawStr(x1, y1, tempStr);
  u8g2.drawStr(x2, y2, tempStr);
  u8g2.sendBuffer();
}
