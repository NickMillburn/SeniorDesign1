#include <Arduino.h>
#include <math.h>

namespace {

constexpr uint8_t analogPin = A0;
constexpr uint8_t ledPin = LED_BUILTIN;
constexpr size_t numSections = 2;
constexpr size_t historyLength = 10;
constexpr unsigned long samplePeriodUs = 250;
constexpr unsigned long reportPeriodUs = 1000000UL;
constexpr float threshold = 0.2f;
constexpr float adcMidpoint = 2.5f;
constexpr float adcScale = 5.0f / 1023.0f;

struct SosSection {
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
    float d1;
    float d2;
};

SosSection g_sections[numSections] = {
    {1.0f, 0.0f, -1.0f, -1.4682095f, 0.93292826f, 0.0f, 0.0f},
    {1.0f, 0.0f, -1.0f, -1.5529847f, 0.93813700f, 0.0f, 0.0f},
};

constexpr float sectionScales[numSections] = {
    0.045613233f,
    0.045613233f,
};

float g_peakHistory[historyLength] = {};
size_t g_historyIndex = 0;
unsigned long g_lastReportUs = 0;
unsigned long missedSamples = 0;

// Run one ADC sample through the cascaded SOS filter.
float runFilter(float sample) {
    float stageValue = sample;

    for (size_t i = 0; i < numSections; ++i) {
        SosSection &section = g_sections[i];
        const float scaledInput = stageValue * sectionScales[i];
        const float output = section.b0 * scaledInput + section.d1;

        section.d1 = section.b1 * scaledInput - section.a1 * output + section.d2;
        section.d2 = section.b2 * scaledInput - section.a2 * output;
        stageValue = output;
    }

    return stageValue;
}

// Store the latest detector magnitude in a ring buffer.
void pushMagnitude(float magnitude) {
    g_peakHistory[g_historyIndex] = magnitude;
    g_historyIndex = (g_historyIndex + 1U) % historyLength;
}

// Return the peak magnitude seen in the recent history window.
float maxRecentMagnitude() {
    float maxValue = 0.0f;

    for (float value : g_peakHistory) {
        if (value > maxValue) {
            maxValue = value;
        }
    }

    return maxValue;
}

}  // namespace

// Initialize serial output and the detector status LED.
void setup() {
    Serial.begin(230400);
    pinMode(ledPin, OUTPUT);
}

// Sample, filter, update the detector output, and hold the sample rate.
void loop() {
    const unsigned long sampleStartUs = micros();
    const int rawSample = analogRead(analogPin);
    const float centeredSample = rawSample * adcScale - adcMidpoint;
    const float filterOutput = runFilter(centeredSample);

    pushMagnitude(fabsf(2.0f * filterOutput));

    const unsigned long nowUs = micros();
    if (nowUs - g_lastReportUs >= reportPeriodUs) {
        const float peakMagnitude = maxRecentMagnitude();
        Serial.print(peakMagnitude > threshold ? "Detecting " : "Not detecting ");
        Serial.print("peak=");
        Serial.print(peakMagnitude, 6);
        Serial.print(" missed=");
        Serial.println(missedSamples);
        digitalWrite(ledPin, peakMagnitude < threshold ? HIGH : LOW);
        g_lastReportUs = nowUs;
        missedSamples = 0;
    }

    if (micros() - sampleStartUs > samplePeriodUs) {
        missedSamples++;
        return;
    }

    while (micros() - sampleStartUs < samplePeriodUs) {
    }
}
