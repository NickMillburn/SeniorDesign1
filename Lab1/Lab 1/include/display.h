#pragma once
#include <Arduino.h>

// init the display hardware + library
void display_init();

// Update the display with current sensor info.
// btn1/btn2: true if that sensor's button is pressed (on)
// tempC1/tempC2: temperature in Celsius from sensors_getTempC()
void display_update(bool btn1, float tempC1, bool btn2, float tempC2);
