#include <OneWire.h>
#include <DallasTemperature.h>
#include "sensors.h"

#define SENSOR_COUNT 2

static OneWire oneWire0(D2);
static OneWire oneWire1(D3);

static DallasTemperature sensor0(&oneWire0);
static DallasTemperature sensor1(&oneWire1);

static DallasTemperature* sensors[SENSOR_COUNT] = { &sensor0, &sensor1 };

void sensors_init() {
    sensor0.begin();
    sensor1.begin();
}

void sensors_update() {
    sensor0.requestTemperatures();
    sensor1.requestTemperatures();
}

float sensors_getTempC(int sensor) {
    if (sensor < 0 || sensor >= SENSOR_COUNT) return DEVICE_DISCONNECTED_C;
    return sensors[sensor]->getTempCByIndex(0);
}

float sensors_getTempF(int sensor) {
    if (sensor < 0 || sensor >= SENSOR_COUNT) return DEVICE_DISCONNECTED_F;
    return sensors[sensor]->getTempFByIndex(0);
}
