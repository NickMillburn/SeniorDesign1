# Lab 2 — Filter Design & Tone Detection

### **Purpose:**
Design and implement a digital filter on the Arduino Uno R4 WiFi that isolates a target tone from
an analog input signal, then uses the filtered output to drive a real-time detector. The prototype
demonstrates that the filter meets the passband/stopband requirements laid out in the lab documents
and that the detector reliably reports when the tone is present.

[Lab Requirements](https://github.com/NickMillburn/SeniorDesign1/tree/main/Lab2/Requirements)

### **Hardware Used**
<ul>
    <li>1x Arduino Uno R4 WiFi</li>
    <li>Analog signal input on pin A0 (see transmitter/receiver circuit schematics in Requirements)</li>
    <li>1x Onboard LED (detection indicator)</li>
</ul>

The `Requirements` folder contains the lab handout, the filter-design assignment, and the
transmitter/receiver circuit schematics.

### **How It Works**
1. The ADC samples pin A0 at a fixed 250 µs period (4 kHz), centering each reading around the 2.5 V midpoint.
2. Each sample is run through a cascaded **second-order-section (biquad) IIR filter** — two SOS stages with precomputed coefficients — to isolate the target tone band.
3. The rectified filter output is stored in a short ring buffer; the peak magnitude over the recent window is compared against a detection threshold.
4. When the tone is detected the onboard LED turns on; when detection *stops*, the board sends an email alert (via SendGrid) timestamped with the current time.
5. The hardware RTC is kept accurate by syncing to an NTP server at boot and re-syncing hourly to correct drift.

### **Software Architecture**
PlatformIO project targeting the `uno_r4_wifi` board. Source lives in `Lab2/src` with matching headers in `Lab2/include`:

| File | Responsibility |
| --- | --- |
| `ESP.cpp` | Main program: ADC sampling loop, the cascaded SOS filter, detection logic, and LED output. |
| `timegetter.cpp` | NTP client that queries `pool.ntp.org` and writes the result into the hardware RTC (with a configurable UTC offset). |
| `messenger.cpp` | Formats a timestamped alert and sends it through the SendGrid API. |

### **Setup / Build**
1. Install [PlatformIO](https://platformio.org/) (VS Code extension or CLI).
2. Provide Wi-Fi credentials and the SendGrid API key in a `Lab2/src/.env` file (git-ignored). It is `#include`d directly by the source and must define `PRIVATE_SSID`, `PRIVATE_PASSWORD`, and `SENDGRID_API_KEY`.
3. Build and upload from the `Lab2` directory: `pio run --target upload`.
4. Open the serial monitor at 230400 baud to watch the detector output (peak magnitude and missed-sample count).

> **Note:** The `.env` file holds Wi-Fi credentials and the SendGrid API key and is intentionally excluded from version control. Never commit real credentials or API keys.
