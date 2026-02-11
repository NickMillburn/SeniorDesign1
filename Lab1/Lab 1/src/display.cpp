#include <Wire.h>
#include <U8g2lib.h>
#include "display.h"

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void display_init() {
  Wire.begin();
  u8g2.begin();
}

void display_show_default() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_logisoso32_tf);

  const char *text = "NICK";
  int16_t x = (128 - u8g2.getStrWidth(text)) / 2;
  int16_t y = 52;

  u8g2.drawStr(x, y, text);
  u8g2.sendBuffer();
}
