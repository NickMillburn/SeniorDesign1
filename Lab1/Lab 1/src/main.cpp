#include <OneWire.h>
#include <DallasTemperature.h>

constexpr uint8_t PIN_A = 2;   // Sensor A DQ
constexpr uint8_t PIN_B = 3;   // Sensor B DQ

OneWire oneWireA(PIN_A);
OneWire oneWireB(PIN_B);

DallasTemperature sensorA(&oneWireA);
DallasTemperature sensorB(&oneWireB);

void setup() {
  Serial.begin(115200);

  
  pinMode(PIN_A, INPUT_PULLUP); 
  pinMode(PIN_B, INPUT_PULLUP);

  sensorA.begin();
  sensorB.begin();
}

void loop() {
  sensorA.requestTemperatures();
  sensorB.requestTemperatures();

  float tA = sensorA.getTempCByIndex(0);
  float tB = sensorB.getTempCByIndex(0);

  Serial.print("A: "); Serial.print(tA); Serial.print(" C   ");
  Serial.print("B: "); Serial.print(tB); Serial.println(" C");

  delay(1000);
}
