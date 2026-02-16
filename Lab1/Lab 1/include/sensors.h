#pragma once
#include <Arduino.h>

// Initialize the temperature sensors (call once in setup)
void sensors_init();

// Request fresh readings from both sensors (call before reading temps)
void sensors_update();

// Get temperature from a sensor (0 = sensor on D2, 1 = sensor on D3)
float sensors_getTempC(int sensor);
float sensors_getTempF(int sensor);
