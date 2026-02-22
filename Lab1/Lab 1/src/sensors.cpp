#include <OneWire.h>
#include <DallasTemperature.h>
#include "sensors.h"

#define SENSOR_COUNT 2
#define CONVERSION_DELAY_MS 1000  // 1 reading per second

static unsigned long lastRequestTime = 0;
static bool conversionPending = false;

static OneWire oneWire0(D2);
static OneWire oneWire1(D3);

static DallasTemperature sensor0(&oneWire0);
static DallasTemperature sensor1(&oneWire1);

static DallasTemperature* sensors[SENSOR_COUNT] = { &sensor0, &sensor1 };

void sensors_init() {
    sensor0.begin();
    sensor1.begin();

    sensor0.setWaitForConversion(false);
    sensor1.setWaitForConversion(false);

    sensor0.requestTemperatures();
    sensor1.requestTemperatures();
    lastRequestTime = millis();
    conversionPending = true;
}

void sensors_update() {
    unsigned long now = millis();

    if (conversionPending) {
        if ((now - lastRequestTime) >= CONVERSION_DELAY_MS) {
            conversionPending = false;
        }
    }

    if (!conversionPending) {
        sensor0.requestTemperatures();
        sensor1.requestTemperatures();
        lastRequestTime = now;
        conversionPending = true;
    }
}

float sensors_getTempC(int sensor) {
    if (sensor < 0 || sensor >= SENSOR_COUNT) return DEVICE_DISCONNECTED_C;
    return sensors[sensor]->getTempCByIndex(0);
}

float sensors_getTempF(int sensor) {
    if (sensor < 0 || sensor >= SENSOR_COUNT) return DEVICE_DISCONNECTED_F;
    return sensors[sensor]->getTempFByIndex(0);
}
