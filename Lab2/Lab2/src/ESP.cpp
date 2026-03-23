#include <Arduino.h>

// A digital frequency selective filter
// A. Kruger, 2019
// revised R. Mudumbai, 2020 & 2024
// revised N. Najeeb, 2025

int analogPin = A0;
int LED = 12;

const int n = 7;
int m = 10;

float den[] = {1.0000, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00};
float num[] = {0.0058, 0.00, -0.015, 0.00, 0.015, 0.00, -0.0058};

float x[n], y[n], filter_out, s[10];  // renamed yn -> filter_out

float threshold_val = 0.2;
int Ts = 333;

void setup()
{
    Serial.begin(9600);  // bumped up from 1200, RA4M1 can handle it
    int i;

    // REMOVED: AVR-specific ADC prescaler lines, not needed on RA4M1

    pinMode(LED, OUTPUT);

    for (i = 0; i < n; i++)
        x[i] = y[i] = 0;

    for (i = 0; i < m; i++)
        s[i] = 0;
    filter_out = 0;
}

void loop()
{
    unsigned long t1;
    int i, count, val;
    float changet = micros();

    count = 0;
    while (1) {
        t1 = micros();

        for (i = n-1; i > 0; i--) {
            x[i] = x[i-1];
            y[i] = y[i-1];
        }

        for (i = m-1; i > 0; i--)
            s[i] = s[i-1];

        val = analogRead(analogPin);
        x[0] = val * (5.0 / 1023.0) - 2.5;

        filter_out = num[0] * x[0];

        for (i = 1; i < n; i++)
            filter_out = filter_out - den[i] * y[i] + num[i] * x[i];

        y[0] = filter_out;

        s[0] = abs(2 * filter_out);

        float maxs = 0;
        for (int i = 0; i < m; i++) {
            if (s[i] > maxs)
                maxs = s[i];
        }

        if ((micros() - changet) > 1000e3)
        {
            Serial.println(maxs);
            changet = micros();

            if (maxs < threshold_val)
                digitalWrite(LED, HIGH);
            else
                digitalWrite(LED, LOW);
        }

        if ((micros() - t1) > Ts)
            Serial.println("MISSED A SAMPLE");

        while ((micros() - t1) < Ts);
    }
}