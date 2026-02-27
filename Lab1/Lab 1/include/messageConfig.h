#include <Arduino.h>

// Holds all user-configurable alert settings.
struct messageConfig {
    String  recipient       = "tpurcell@uiowa.edu";
    String  subject         = "Temperature Sensor Notification";
    String  bodyTemplate    = "Sensor {sensor} reached {temp} deg C.";
    float   maxThresholdC   = 50.0f;   // send alert when temp rises ABOVE this
    float   minThresholdC   = 10.0f;    // send alert when temp falls BELOW this
    bool    alertEnabled    = true;
};