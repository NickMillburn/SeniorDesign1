#include "phys_input.h"

//Debounce configuration
#define DEBOUNCE_MS 50

//Global state (declared extern in header)
volatile bool sensor1Active = false;  // Sensor 1 toggle: off by default
volatile bool sensor2Active = false;  // Sensor 2 toggle: off by default
volatile bool systemPowerOn = false;  // Power switch: read at startup

// Internal debounce state for each pushbutton
// We only debounce the pushbuttons (toggle on press).
// The power switch is a latching slide switch — read directly, no debounce needed.
static unsigned long lastDebounceTime1 = 0;
static unsigned long lastDebounceTime2 = 0;
static bool lastStableState1 = HIGH;  // active-low: HIGH = not pressed
static bool lastStableState2 = HIGH;

void phys_input_init() {
    // All inputs use internal pull-ups; buttons/switch connect to GND when active
    pinMode(PIN_BUTTON1,  INPUT_PULLUP);
    pinMode(PIN_BUTTON2,  INPUT_PULLUP);
    pinMode(PIN_POWER_ON,  INPUT_PULLUP);
    pinMode(PIN_POWER_OFF, INPUT_PULLUP);

    // Read initial power switch position
    systemPowerOn = (digitalRead(PIN_POWER_ON) == LOW);  // active-low
}

// Helper: debounce a momentary pushbutton and toggle a flag on press
// Returns true if the button was just pressed (falling edge after debounce).
static void debounce_toggle(uint8_t pin,
                            bool &lastStable,
                            unsigned long &lastDebTime,
                            volatile bool &toggleFlag)
{
    bool reading = digitalRead(pin);  // LOW = pressed (active-low)
    unsigned long now = millis();

    // If the raw reading changed, reset the debounce timer
    if (reading != lastStable) {
        // Only accept the new state after it has been stable for DEBOUNCE_MS
        if ((now - lastDebTime) >= DEBOUNCE_MS) {
            // State has been stable long enough -> accept it
            bool prevStable = lastStable;
            lastStable = reading;
            lastDebTime = now;

            // Detect falling edge (HIGH → LOW) = button just pressed
            if (prevStable == HIGH && reading == LOW) {
                toggleFlag = !toggleFlag;  // flip the toggle
            }
        }
    } else {
        // Reading matches last stable state keep resetting the timer
        // so the next *different* reading starts a fresh debounce window
        lastDebTime = now;
    }
}

void phys_input_update() {
    // Power switch: only change state when a pin is actively LOW
    bool onReading  = (digitalRead(PIN_POWER_ON)  == LOW);
    bool offReading = (digitalRead(PIN_POWER_OFF) == LOW);

    if (onReading) {
        // Detect OFF→ON transition: reset toggle states so user starts clean
        if (!systemPowerOn) {
            sensor1Active = false;
            sensor2Active = false;
        }
        systemPowerOn = true;
    } else if (offReading) {
        systemPowerOn = false;
    }
    // Middle position: neither LOW, keep previous state

    // Only process buttons when the system is powered on
    if (!systemPowerOn) return;

    //Pushbutton debounce + toggle
    debounce_toggle(PIN_BUTTON1, lastStableState1, lastDebounceTime1, sensor1Active);
    debounce_toggle(PIN_BUTTON2, lastStableState2, lastDebounceTime2, sensor2Active);
}
