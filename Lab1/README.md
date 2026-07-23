# Lab 1 — Arduino Temperature Sensor with Web Interface

### **Purpose:**
You are to design a thermometer with a web interface. The "design" is a set of documents that
describes how to make this device. The prototype accompanying this design demonstrates that the
design works.

This design must satisfy a set of exact requirements, as laid out in this document. Some of these
requirements specify the expected functionality and performance of the design, and some are
mechanical specifications. The primary challenge of this project is to analyze the requirements
carefully and create and implement a design that satisfies all of them. As you read through
this document, you may experience some uncertainty about the meaning or interpretation of certain
requirements. You must clarify all such uncertainties before finalizing your design.

This assignment should be completed in "rapid prototyping" fashion, with tangible results expected
early on. With this design technique, the initial prototype may not work quite right or fully
implement all functionalities, but it does work in some fashion. After the initial prototype is
working, features are added, or designs are reworked to meet all the requirements by the end of
the project schedule.

[Full Detail Project Description](https://github.com/NickMillburn/SeniorDesign1/blob/main/Lab1/Lab1_2026Spring.pdf)

### **Hardware Used**
<ul>
    <li>1x Arduino Uno R4 WiFi</li>
    <li>2x DS18B20A Digital Temperature Sensor</li>
    <li>1x SSD1306 OLED Display (128x64, I2C)</li>
    <li>1x 3-position Slide/Toggle Switch (power on / off)</li>
    <li>2x Momentary Pushbuttons (per-sensor enable/disable)</li>
</ul>

[Datasheets](https://github.com/NickMillburn/SeniorDesign1/tree/main/Lab1/Datasheets)

### **Features**
- Reads two DS18B20 one-wire temperature sensors, updating roughly once per second.
- Local OLED display shows each sensor's temperature, an "off" state, or an "ERROR" state when a sensor is disconnected.
- Physical controls: a slide switch powers the system on/off and two debounced pushbuttons toggle each sensor independently.
- Hosts a Wi-Fi web server that serves a live dashboard (Chart.js) with a rolling temperature history, C/F unit switching, per-sensor on/off controls, and per-sensor/both view modes.
- Configurable high/low temperature thresholds and a recipient email address, editable from the web UI.
- Sends email alerts via the SendGrid API when a reading crosses a threshold, with a per-sensor cooldown to prevent spamming.

### **Software Architecture**
The firmware is a PlatformIO project targeting the `uno_r4_wifi` board. Source lives in `Lab 1/src` with matching headers in `Lab 1/include`:

| File | Responsibility |
| --- | --- |
| `main.cpp` | Boot/Wi-Fi setup, main loop, wiring the sensor, display, input, server, and messenger modules together. |
| `sensors.cpp` | Non-blocking DS18B20 reads via OneWire/DallasTemperature (1 conversion/second). |
| `display.cpp` | SSD1306 OLED rendering using the U8g2 library. |
| `phys_input.cpp` | Power slide switch and debounced pushbutton handling; owns the shared power/sensor state flags. |
| `TempServer.cpp` | HTTP server: serves the dashboard HTML/JS, the `/data` JSON endpoint, and the control/settings endpoints. |
| `messanger.cpp` | Threshold checks and SendGrid email delivery with per-sensor cooldown. |

### **Setup / Build**
1. Install [PlatformIO](https://platformio.org/) (VS Code extension or CLI).
2. Provide Wi-Fi credentials in `Lab 1/include/network_credentials.h` (this file is git-ignored). Define `PRIVATE_SSID` and `PRIVATE_PASSWORD`.
3. Provide a SendGrid API key in a `Lab 1/.env` file (git-ignored). Copy `.env.example` and set `SENDGRID_API_KEY`. The `scripts/load_env.py` pre-build script injects it as a compile-time define.
4. Build and upload from the `Lab 1` directory: `pio run --target upload`.
5. Open the serial monitor at 115200 baud to see the board's IP address, then browse to it to reach the dashboard.

> **Note:** `network_credentials.h` and `.env` hold secrets and are intentionally excluded from version control. Never commit real credentials or API keys.
