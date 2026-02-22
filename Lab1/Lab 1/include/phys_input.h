#pragma once
#include <Arduino.h>

//Pin assignments
#define PIN_BUTTON1  A3  // Pushbutton 1 controls Sensor 1 display
#define PIN_BUTTON2  A2   // Pushbutton 2 controls Sensor 2 display
#define PIN_POWER_ON  D9   // Power switch ON position
#define PIN_POWER_OFF D10  // Power switch OFF position

// Global state flags
// Toggle states for each sensor's display (true = show temp, false = show "off").
// Readable and writable by other modules (web server, display driver, etc.).
extern volatile bool sensor1Active;
extern volatile bool sensor2Active;

// Power switch state (true = system on). Read-only for other modules;
// updated only by phys_input_update().
extern volatile bool systemPowerOn;

// Initialize all input pins (call once in setup)
void phys_input_init();

// Poll buttons and switch, apply debounce, update global state flags.
// Call once per loop() iteration.
void phys_input_update();
