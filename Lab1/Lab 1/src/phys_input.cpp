#include "phys_input.h"

//Global state (declared extern in header)
volatile bool sensor1Active = false;  // Sensor 1 toggle: off by default
volatile bool sensor2Active = false;  // Sensor 2 toggle: off by default
volatile bool systemPowerOn = false;  // Power switch: read at startup

static bool lastReading1 = HIGH;  // active-low: HIGH = not pressed
static bool lastReading2 = HIGH;

void phys_input_init() {
    pinMode(PIN_BUTTON1,  INPUT);
    pinMode(PIN_BUTTON2,  INPUT);
    pinMode(PIN_POWER_SW, INPUT_PULLUP);

    // Read initial power switch position
    systemPowerOn = (digitalRead(PIN_POWER_SW) == LOW);  // active-low
}

static void edge_toggle(uint8_t pin, bool &lastRead, volatile bool &toggleFlag) {
    bool reading = digitalRead(pin);
    if (lastRead == HIGH && reading == LOW) {
        toggleFlag = !toggleFlag;
    }
    lastRead = reading;
}

void phys_input_update() {
    // Power switch (latching)
    bool powerReading = (digitalRead(PIN_POWER_SW) == LOW);  // active-low

    // Detect OFF→ON transition: reset toggle states so user starts clean
    if (powerReading && !systemPowerOn) {
        sensor1Active = false;
        sensor2Active = false;
    }
    systemPowerOn = powerReading;

    // Only process buttons when the system is powered on
    if (!systemPowerOn) return;

    edge_toggle(PIN_BUTTON1, lastReading1, sensor1Active);
    edge_toggle(PIN_BUTTON2, lastReading2, sensor2Active);
}
